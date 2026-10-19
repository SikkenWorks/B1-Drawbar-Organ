/* SPDX-License-Identifier: GPL-3.0-only
 * SikkenWorks B1 V4.1: full V4.0 organ/rock DSP with exact Smart Idle fast path.
 * This is original DSP informed by published Hammond service explanations.
 * NOT a sampled Hammond, not a perfect circuit-level model; verify by listening.
 * All work in audio callback is bounded, fixed-point, allocation-free.
 */
#include "b1_engine.h"
#include "b1_tables.h"
#include "b1_softclip_lut.h"
#include "b1_guard_tables.h"
#include "b1_saw_tables.h"
#include "b1_v34_shaping_tables.h"

/* Semi-tone intervals at nine historic Hammond B3 drawbar footages.
 * 16', 5 1/3', 8', 4', 2 2/3', 2', 1 3/5', 1 1/3', 1'. */
static const int8_t b3_offset[9]={-12,7,0,12,19,24,28,31,36};
/* Approximately -3 dB per drawbar detent; upper values are not linear. */
static const uint16_t pull_q8[9]={0,23,32,45,64,91,128,181,256};
static int16_t clamp16(int32_t n){return (int16_t)(n>32767?32767:n< -32768?-32768:n);}
/* Realtime-safe exact absolute value as unsigned magnitude.
 * The previous (int64_t) negation in four hot DSP paths can pull in
 * emulated 64-bit arithmetic on pi32v2. This handles INT32_MIN exactly
 * without signed overflow and preserves every Q-format output. */
static uint32_t b1_abs32(int32_t n){
 uint32_t bits=(uint32_t)n;
 return n<0?0u-bits:bits;
}
/* A deterministic, non-sampled per-wheel profile. Small odd/even partials
 * imitate variations across a tonewheel/generator, but are not intended as an
 * exact frequency response of any one vintage B3. Drop ultrasonic partials. */
static int32_t wheel_sample(const struct b1_engine *e,uint8_t wheel){
 uint32_t phase=e->wheel_phase[wheel];
 uint16_t blend=e->osc_blend_q15;
 int32_t organ=0;
 /* At full SAW blend skip even the sine lookup. While ORGAN is selected,
  * preserve the existing V3.2 wave calculation exactly (all DSP paths). */
 if(blend!=32767u){
  uint32_t inc=e->wheel_inc[wheel];
  organ=b1_sine[phase>>21];
  if(!e->cpu_relief){
   int32_t even=b1_wheel_even[wheel];
   int32_t odd=b1_wheel_odd[wheel];
   if(inc<0x40000000u)organ+=((int32_t)b1_sine[(phase>>20)&2047u]*even)/288;
   if(inc<0x2aaaaaaau)organ+=((int32_t)b1_sine[((phase>>21)*3u)&2047u]*odd)/380;
   organ=organ*(int32_t)b1_wheel_gain[wheel]/256;
  }
 }
 if(!blend)return organ;
 /* SAW is frequency-banded to avoid the harsh aliasing of a raw step.
  * One precomputed lookup per shared wheel; linear interpolation prevents
  * zipper distortion. Upper partial cap selected once on reset. */
 uint32_t index=phase>>23; /* 512 samples / cycle */
 uint32_t frac=(phase>>15)&255u;
 const int16_t *table=b1_saw_lut[e->osc_band[wheel]];
 int32_t a=table[index],b=table[(index+1u)&511u];
 int32_t saw=a+(((b-a)*(int32_t)frac)>>8);
 if(blend==32767u)return saw;
 return organ+(((saw-organ)*(int32_t)blend)>>15);
}
/* A tonewheel used by multiple notes/harmonics is physically a *shared*
 * oscillator. Memoize one evaluation per sample while leaving the oscillator
 * phases free-running and the audible sample value unchanged. */
static int32_t shared_wheel_sample(struct b1_engine *e,uint8_t wheel){
 if(e->wheel_stamp[wheel]!=e->wheel_frame){
  /* Phase-accurate LAZY advancing: the 91 tonewheels keep running, but
   * each phase is updated only when this wheel contributes to the sound.
   * Avoid 91 unconditional phase additions per 44.1kHz sample. Wrapsafe
   * unsigned frame differences preserve a continuously running wheel. */
  uint32_t elapsed=e->wheel_frame-e->wheel_phase_stamp[wheel];
  /* The common consecutive sample costs one native add, not an
   * unnecessary 32x32 multiply. Long-idle/wrap intervals retain the
   * original exact modulo-2^32 phase jump. */
  if(elapsed==1u)e->wheel_phase[wheel]+=e->wheel_inc[wheel];
  else e->wheel_phase[wheel]+=e->wheel_inc[wheel]*elapsed;
  e->wheel_phase_stamp[wheel]=e->wheel_frame;
  e->wheel_value[wheel]=wheel_sample(e,wheel);
  e->wheel_stamp[wheel]=e->wheel_frame;
 }
 return e->wheel_value[wheel];
}
/* Table-interpolated x*knee/(knee+abs(x)). No 64-bit divide in the
 * normal audio path. Tables are generated from the original integer formula;
 * deviations remain small and are verified against golden offline renders.
 * Rare out-of-range signals use exact fallback instead of a hard plateau. */
/* Interpolated Hammond scanner: integer-delay stepping produced audible zipper\n * noise on vibrato and chorus. Q8 fractional reads are continuous. */
static int32_t scanner_process(struct b1_engine *e,int32_t dry,int32_t depth_q8){
 uint16_t target=0u;
 if(e->cv_mode && e->cv_mode<=6u && e->cv_depth)
  target=e->cv_mode<=3u?32767u:16384u;
 if(e->cv_wet_q15<target){
  uint32_t n=(uint32_t)e->cv_wet_q15+96u;
  e->cv_wet_q15=(uint16_t)(n>target?target:n);
 }else if(e->cv_wet_q15>target){
  e->cv_wet_q15=(uint16_t)((uint32_t)(e->cv_wet_q15-target)>96u?e->cv_wet_q15-96u:target);
 }
 /* Avoid heavy scanner interpolation when completely bypassed. */
 if(!e->cv_wet_q15){
  e->cv_line[e->cv_write]=clamp16(dry);
  e->cv_write=(e->cv_write+1u)&(B1_CV_SAMPLES-1u);
  return dry;
 }
 e->cv_phase+=505000u; /* approximately 5.2 Hz */
 uint32_t type=e->cv_mode?((e->cv_mode-1u)%3u):0u;
 (void)type; /* depth_q8 was precomputed outside the PCM sample loop */
 int32_t mod_q8=(b1_sine[e->cv_phase>>21]*depth_q8)/32768;
 int32_t delay_q8=(155*256)+mod_q8;
 if(delay_q8<32*256)delay_q8=32*256;
 if(delay_q8>448*256)delay_q8=448*256;
 uint32_t at=(e->cv_write-((uint32_t)delay_q8>>8))&(B1_CV_SAMPLES-1u);
 uint32_t frac=(uint32_t)delay_q8&255u;
 int32_t wet=(e->cv_line[at]*(int32_t)(256u-frac)+
  e->cv_line[(at-1u)&(B1_CV_SAMPLES-1u)]*(int32_t)frac)>>8;
 e->cv_line[e->cv_write]=clamp16(dry);
 e->cv_write=(e->cv_write+1u)&(B1_CV_SAMPLES-1u);
 /* Interpolate Q15 using a shift, not two 64-bit products and a
  * 64-bit divide PER AUDIO SAMPLE. The +1 LSB denominator difference
  * is well below 16-bit resolution. */
 /* Input guard bounds DRY to the 15k-ish working range and WET is
  * a clamped 16-bit delay tap. The maximum intermediate magnitude is
  * therefore < 48k * 32767 (< INT32_MAX): a native signed multiply avoids
  * costly 64-bit emulation in the 60MHz callback, with identical Q15 PCM. */
 return dry+(((wet-dry)*(int32_t)e->cv_wet_q15)>>15);
}
/* 4 musical spaces built from bounded integer combs and an all-pass diffusion
 * stage. 0=OFF 1=SPRING 2=ROOM 3=HALL 4=CATHEDRAL. No heap/locks. */
/* V2.9: fixed FX coefficients are calculated ONCE per audio DMA block,
 * not 44,100 times a second. The engine controls are constant during a
 * render call; UI writes occur outside the audio interrupt. The same
 * integer formulas and signal path are retained, including stereo tails. */
struct b1_block_fx {
 uint32_t delay_tap;
 int32_t damping,fb;
 uint32_t damping_recip;
 uint8_t delay_spread,reverb_type;
};
static struct b1_block_fx b1_prepare_fx(const struct b1_engine *e){
 struct b1_block_fx f;
 f.delay_tap=1400u+(uint32_t)e->delay_time*140u;
 f.delay_spread=e->delay_spread;
 f.reverb_type=e->reverb_type;
 f.damping=2+(int32_t)e->reverb_damp/14;
 f.damping_recip=65536u/(uint32_t)f.damping;
 f.fb=(int32_t)(e->reverb_type==1u?76u:e->reverb_type==2u?115u:e->reverb_type==3u?152u:187u);
 f.fb+=(int32_t)e->reverb_decay/2;
 if(f.fb>232)f.fb=232;
 return f;
}
/* Exact signed division through a Q16 reciprocal, calculated just ONCE per
 * 128-frame block. A 32x32 multiply, shift, and one correction handle signed
 * truncated-toward-zero results for any int16 ring difference. The ISR no
 * longer invokes a runtime 32-bit divide three times per audio sample.
 * Bounds: int16 tap - bounded IIR state is within [-65535,+65535]. */
static inline int32_t b1_reverb_div(int32_t x,int32_t damping,uint32_t reciprocal){
 uint32_t mag=b1_abs32(x);
 if(mag>65535u)return x/damping; /* defensive fallback for corrupt state */
 uint32_t q=(mag*reciprocal)>>16;
 if((q+1u)*(uint32_t)damping<=mag)++q;
 return x<0?-(int32_t)q:(int32_t)q;
}
static void space_process(struct b1_engine *e,int32_t *left,int32_t *right,const struct b1_block_fx *f){
 if(!e->delay_mix && (!e->reverb_type || !e->reverb_mix))return;
 int32_t mono=(*left+*right)/2;
 uint32_t i=e->delay_write++&(B1_DELAY_SAMPLES-1u);
 int32_t wet_d=0;
 if(e->delay_mix){
  uint32_t tap=f->delay_tap; /* ~32 to ~350ms */
  uint32_t j=(i-tap)&(B1_DELAY_SAMPLES-1u);
  wet_d=e->delay_line[j];
  int32_t feedback=(wet_d*(int32_t)e->delay_feedback)/130;
  e->delay_line[i]=clamp16(mono/2+feedback);
  int32_t spread=(int32_t)f->delay_spread;
  int32_t l=wet_d*(100-spread/3)/100;
  int32_t r=wet_d*(80+spread/5)/100;
  *left+=l*(int32_t)e->delay_mix/100;
  *right+=r*(int32_t)e->delay_mix/100;
 }else e->delay_line[i]=0;
 if(e->reverb_type && e->reverb_mix){
  uint32_t idx=e->rev_write++;
  uint8_t type=f->reverb_type;
  /* Spring: short metallic repetitions. Room/hall/cathedral add increasingly
     long, damped feedback. All feedback gains bounded strictly below 1. */
  int32_t fb=f->fb;
  uint32_t ai=idx&(B1_REVERB_A-1u),bi=idx&(B1_REVERB_B-1u);
  uint32_t ci=idx&(B1_REVERB_C-1u),di=idx&(B1_REVERB_DIFF-1u);
  int32_t a=e->rev_a[ai],b=e->rev_b[bi],c=e->rev_c[ci];
  /* Each comb has a different effective delay using three full rings. */
  int32_t damping=f->damping;
  e->rev_damp_a+=b1_reverb_div(a-e->rev_damp_a,damping,f->damping_recip);
  e->rev_damp_b+=b1_reverb_div(b-e->rev_damp_b,damping,f->damping_recip);
  e->rev_damp_c+=b1_reverb_div(c-e->rev_damp_c,damping,f->damping_recip);
  int32_t input=mono/4;
  e->rev_a[ai]=clamp16(input+e->rev_damp_a*fb/256);
  e->rev_b[bi]=clamp16(input+e->rev_damp_b*fb/256);
  e->rev_c[ci]=clamp16(input+e->rev_damp_c*fb/256);
  int32_t sum=(a+b+c)/3;
  int32_t diff=e->rev_diff[di];
  e->rev_diff[di]=clamp16(sum+diff/3);
  int32_t wet=(sum-diff/3);
  int32_t blend=wet*(int32_t)e->reverb_mix/100;
  *left+=blend;
  *right+=((type==1u?diff:wet)*((int32_t)e->reverb_mix))/110;
 }
}
static int32_t soft_ratio(int32_t x,const uint16_t *tbl,uint32_t knee){
 uint32_t a=b1_abs32(x);
 uint32_t y;
 if(a>=1048576u){
  y=(uint32_t)(((uint64_t)a*knee)/(knee+a));
 }else{
  uint32_t at=a>>9,frac=a&511u;
  /* V3.9: LUT values are monotone. a*(512-frac)+b*frac equals
   * 512*a+(b-a)*frac, so only ONE 32-bit multiply per softclip
   * instead of two, with exactly the same integer rounding. The three
   * generated clip tables are exhaustively checked by the native test. */
  uint32_t base=tbl[at];
  uint32_t step=(uint32_t)tbl[at+1u]-base;
  y=base+((step*frac)>>9);
 }
 return x<0?-(int32_t)y:(int32_t)y;
}
/* V2.5: Never let a polyphonic organ sum force the scanner, cabinet or
 * amplifier into permanent clipping.  Real drawbar tonewheels are summed
 * linearly.  A peak follower lowers gain BEFORE nonlinear stages, linked to
 * every registration, with immediate attenuation and very slow recovery.
 * Division happens only when a new envelope peak changes the required gain
 * (and when recovering above the knee), not in the common clean path.
 * This gain is a transient, never part of a saved patch. */
/* Exact signed Q15 multiply using TWO 32-bit halves. The FM1's pi32v2
 * toolchain otherwise lowers signed 64-bit products into library sequences
 * in the 44.1kHz ISR. Full signed 32-bit inputs are handled exactly:
 * x = hi*65536 + lo, so (x*g)>>15 = 2*hi*g + ((lo*g)>>15).
 * Neither 32-bit intermediate overflows for 0 <= g <= 32767. */
static int32_t b1_mul_q15(int32_t x,uint16_t g){
 int32_t hi=x>>16;
 uint32_t lo=(uint32_t)x&65535u;
 return 2*hi*(int32_t)g+(int32_t)((lo*(uint32_t)g)>>15);
}
static int32_t b1_input_guard(struct b1_engine *e,int32_t in){
 uint32_t a=b1_abs32(in);
 uint32_t peak=e->input_guard_peak;
 if(a>=peak){peak=a;}
 else if(peak){uint32_t decay=(peak>>10)+1u;peak=peak>decay?peak-decay:0u;}
 e->input_guard_peak=peak;
 uint32_t target=32767u;
 if(peak>15000u){
  uint32_t bucket=(peak+255u)>>8;
  target=bucket<1024u?b1_guard_input_q15[bucket]:(uint32_t)(15000u*32767u)/peak;
 }
 if(target<e->input_guard_gain_q15)e->input_guard_gain_q15=(uint16_t)target;
 else if(e->input_guard_gain_q15<target){
  uint32_t next=(uint32_t)e->input_guard_gain_q15+8u;
  e->input_guard_gain_q15=(uint16_t)(next>target?target:next);
 }
 return b1_mul_q15(in,e->input_guard_gain_q15);
}
/* Linked stereo last-resort protection.  Does not soft-clip every normal
 * sample, unlike the previous always-on double-saturation stage.  Fast
 * attack on any transient, slow, smooth recovery; common L/R gain preserves
 * stereo position.  Even intentional ROCK drive cannot overflow the DAC. */
static void b1_output_guard(struct b1_engine *e,int32_t *l,int32_t *r){
 uint32_t a=b1_abs32(*l);
 uint32_t b=b1_abs32(*r);
 uint32_t peak=a>b?a:b;
 uint32_t target=32767u;
 if(peak>27000u){
  uint32_t bucket=(peak+255u)>>8;
  target=bucket<1024u?b1_guard_output_q15[bucket]:(uint32_t)(27000u*32767u)/peak;
 }
 if(target<e->output_guard_gain_q15)e->output_guard_gain_q15=(uint16_t)target;
 else if(e->output_guard_gain_q15<target){
  uint32_t next=(uint32_t)e->output_guard_gain_q15+12u;
  e->output_guard_gain_q15=(uint16_t)(next>target?target:next);
 }
 /* -2.5 dB fixed digital headroom after the linked peak gain.  The old
  * chain peaked safely only because it was continuously soft-clipping; the
  * cleaner route must not suddenly hit the headphone DAC harder. */
 /* Compute the linked gain once. Keep 2.5dB of headroom without a
  * pair of 64-bit integer divides in every 44.1kHz audio frame. */
 uint32_t output_gain_q15=((uint32_t)e->output_guard_gain_q15*3u)>>2;
 *l=b1_mul_q15(*l,(uint16_t)output_gain_q15);
 *r=b1_mul_q15(*r,(uint16_t)output_gain_q15);
}
/* Exact fast path for a rotor/drawbar ramp near its destination.
 * For 0 < abs(target-val) <= tau the old ceiling division equals 1;
 * an integer comparison replaces a 32-bit division on most samples.
 * The full path is retained for large transitions and all values match. */
static uint32_t ramp(uint32_t val,uint32_t target,uint32_t tau){
 if(val==target)return val;
 if(val<target){
  uint32_t diff=target-val;
  if(diff<=tau)return val+1u;
  uint32_t d=(diff+tau-1u)/tau;
  return d>diff?target:val+d;
 }
 uint32_t diff=val-target;
 if(diff<=tau)return val-1u;
 uint32_t d=(diff+tau-1u)/tau;
 return d>diff?target:val-d;
}
/* The 91 wheels run from MIDI C1 (24) through F#8 (114), with lower 16' and
 * high-harmonic octave foldback, as in a classic console organ. */
uint8_t b1_tonewheel_index(uint8_t midi_note,uint8_t footage){
 if(footage>=9u)return 0u;
 int32_t n=(int32_t)midi_note+b3_offset[footage];
 if(footage==0u && n<36)n+=12; /* 16' lowest octave reuses same notes */
 while(n>114)n-=12; /* successive octave foldback of upper harmonics */
 while(n<24)n+=12;
 return (uint8_t)(n-24);
}
static void off_voice(struct b1_engine *e,struct b1_voice *v){
 if(!v->active||v->releasing)return;
 v->releasing=1u;v->release_click_age=0;
 if(v->bank==0u && e->upper_held)e->upper_held--;
 if(e->upper_held==0u){e->percussion_latched=0u;e->percussion_env=0u;}
}
void b1_reset(struct b1_engine *e){
 uint8_t *p=(uint8_t*)e;
 for(uint32_t i=0;i<sizeof(*e);i++)p[i]=0;
 /* Useful B3 registrations: rock upper, comp lower and brighter lead solo. */
 const uint8_t upper[9]={8,8,8,8,0,0,0,0,0}; /* 888800000: classic rock */
 const uint8_t lower[9]={0,0,8,8,0,0,0,0,0};
 const uint8_t solo[9] ={8,8,8,8,8,8,0,0,0};
 for(uint32_t i=0;i<9;i++){
  e->drawbars[0][i]=upper[i];e->drawbars[1][i]=lower[i];e->drawbars[2][i]=solo[i];
  for(uint32_t b=0;b<3u;b++)e->pull_smooth_q16[b][i]=(uint32_t)pull_q8[e->drawbars[b][i]]<<8;
 }
 for(uint32_t i=0;i<B1_TONEWHEELS;i++){
  e->wheel_inc[i]=b1_note_inc[24u+i];
  e->wheel_phase[i]=(uint32_t)(i*0x9e3779b9u+0x2a8f7c63u);
  /* Highest harmonic must fit below ~45% of the sample rate.
   * This initialization runs ONCE outside the DMA audio callback. */
  uint8_t band=0u;
  for(uint8_t j=1u;j<B1_SAW_BANDS;j++)
   if((uint64_t)e->wheel_inc[i]*b1_saw_harmonics[j] < UINT64_C(1932735283))band=j;
  e->osc_band[i]=band;
 }
 e->osc_mode=B1_OSC_ORGAN;e->osc_blend_q15=0u;
 e->rotor_enabled=1u;e->rotor_fast=1u;e->rotor_mix=60u;e->rotor_width=75u;
 e->rotor_slow_pct=100u;e->rotor_fast_pct=100u;e->rotor_drum_pct=100u;
 e->registration_gain_q15=32767u;
 e->input_guard_gain_q15=32767u;e->output_guard_gain_q15=32767u;
 e->rotor_inc=612000u;e->drum_inc=573000u;
 e->percussion_enabled=1u;e->percussion_harmonic=3u;
 /* Musical, conservative startup registration; the player can select the
  * original aggressive ROCK/LORD settings from the existing FX page. */
 e->drive=22u;e->click=0u;e->leakage=5u;
 e->output_gate_q15=0u;e->fx_cleared=1u;
 e->sound_mode=B1_SOUND_B3;e->mode_mix_q15=0u;e->amp_gain=45u;e->amp_tone=62u;e->cabinet_mix=65u;
 e->cv_mode=0u;e->cv_depth=58u;
 e->delay_mix=0u;e->delay_time=45u;e->delay_feedback=30u;e->delay_spread=40u;
 e->reverb_type=0u;e->reverb_mix=0u;e->reverb_decay=50u;e->reverb_damp=60u;
}
void b1_drawbar(struct b1_engine *e,uint8_t bank,uint8_t bar,uint8_t pos){
 if(bank<B1_BANKS && bar<9u)e->drawbars[bank][bar]=pos>8u?8u:pos;
}
void b1_note_on_tagged(struct b1_engine *e,uint8_t note,uint8_t bank,uint8_t velocity,uint32_t owner){
 if(note>127u||bank>=B1_BANKS||!velocity)return;
 uint32_t pick=B1_VOICES;
 for(uint32_t i=0;i<B1_VOICES;i++)if(!e->voice[i].active){pick=i;break;}
 /* Prefer the quietest released voice: avoid cutting a sustaining organ
  * note when its release tail is nearly finished. */
 if(pick==B1_VOICES){
  uint32_t quietest=65536u;
  for(uint32_t i=0;i<B1_VOICES;i++)if(e->voice[i].releasing&&e->voice[i].envelope<quietest){
   quietest=e->voice[i].envelope;pick=i;
  }
 }
 /* Saturated polyphony: steal the oldest sounding voice instead of always
  * cutting voice 0, which disproportionately mutes repeated chord roots. */
 if(pick==B1_VOICES){
  uint32_t age=0u;
  for(uint32_t i=0;i<B1_VOICES;i++)if(e->voice[i].age>=age){age=e->voice[i].age;pick=i;}
 }
 if(pick==B1_VOICES)pick=0u;
 struct b1_voice *v=&e->voice[pick];
 if(v->active&&!v->releasing){if(v->bank==0u && e->upper_held)e->upper_held--;}
 v->note=note;v->bank=bank;v->velocity=velocity;v->owner=owner;
 v->active=1u;v->releasing=0u;v->envelope=0u;v->age=0u;v->release_click_age=0u;
 v->seed=0x92d68ca2u+note*747796405u+owner*2891336453u;
 for(uint32_t h=0;h<9;h++)v->wheel[h]=b1_tonewheel_index(note,(uint8_t)h);
 /* One more precomputed tonewheel address costs one byte/voice, removes
    repeated modulo-91 from the per-voice leakage hot path. */
 v->leakage_wheel=(uint8_t)(v->wheel[2]+12u);
 if(v->leakage_wheel>=B1_TONEWHEELS)v->leakage_wheel-=B1_TONEWHEELS;
 if(bank==0u){
  if(e->upper_held==0u&&!e->percussion_latched&&e->percussion_enabled){
   e->percussion_wheel=b1_tonewheel_index(note,e->percussion_harmonic==2u?3u:4u);
   e->percussion_env=32767u;
   e->percussion_latched=1u;
  }
  if(e->upper_held<255u)e->upper_held++;
 }
}
void b1_note_on(struct b1_engine *e,uint8_t note,uint8_t bank,uint8_t velocity){b1_note_on_tagged(e,note,bank,velocity,0u);}
void b1_note_off(struct b1_engine *e,uint8_t note,uint8_t bank){
 for(uint32_t i=0;i<B1_VOICES;i++){struct b1_voice *v=&e->voice[i];if(v->active&&v->owner==0u&&v->note==note&&v->bank==bank)off_voice(e,v);}
}
void b1_note_off_tagged(struct b1_engine *e,uint32_t owner){
 if(!owner)return;
 for(uint32_t i=0;i<B1_VOICES;i++)if(e->voice[i].active&&e->voice[i].owner==owner)off_voice(e,&e->voice[i]);
}
/* Clear only the audio- path residues, NEVER the free-running wheel phases,
 * registration, settings, or rotor phase. This runs at most once per genuine
 * transition to silence, outside the continuously played voice path. */
static void b1_clear_fx_tail(struct b1_engine *e){
 e->cabinet_hp=0;e->rotor_lp1=0;e->rotor_lp2=0;
 e->amp_hp=0;e->amp_tone_lp=0;e->cab_bass=0;
 e->cab_treble=0;e->cab_presence=0;e->amp_sag=0;
 for(uint32_t i=0;i<B1_CV_SAMPLES;i++)e->cv_line[i]=0;
 for(uint32_t i=0;i<B1_ROTARY_SAMPLES;i++){
  e->rotor_line[i]=0;e->drum_line[i]=0;
 }
 e->cv_wet_q15=0u;e->rev_damp_a=0;e->rev_damp_b=0;e->rev_damp_c=0;
 e->fx_tail_samples=0u;e->fx_was_audible=0u;
 e->fx_cleared=1u;
}
void b1_panic(struct b1_engine *e){
 for(uint32_t i=0;i<B1_VOICES;i++)e->voice[i].active=0u;
 e->upper_held=0u;e->percussion_latched=0u;e->percussion_env=0u;
 e->output_gate_q15=0u;b1_clear_fx_tail(e);
 e->input_guard_peak=0u;e->input_guard_gain_q15=32767u;
 e->output_guard_gain_q15=32767u;
}
static int32_t fractional_read(const int16_t *buf,uint32_t write,uint32_t phase,int32_t center,int32_t delta){
 int32_t dq=center*256+((int32_t)b1_sine[phase>>21]*delta)/32768;
 if(dq<512)dq=512;
 uint32_t age=(uint32_t)dq>>8,frac=(uint32_t)dq&255u;
 uint32_t at=(write-age)&(B1_ROTARY_SAMPLES-1u);
 int32_t a=buf[at],b=buf[(at-1u)&(B1_ROTARY_SAMPLES-1u)];
 return (a*(int32_t)(256u-frac)+b*(int32_t)frac)>>8;
}
static int32_t amp_saturate(int32_t in,uint8_t drive){
 /* Asymmetric soft saturation, restrained at low DRIVE, increasingly dense
  * harmonic distortion for Jon Lord / Marshall-style ROCK registrations. */
 if(drive==0u)return in;
 int32_t gain=256+(int32_t)drive*13; /* fixed Q8: 1 to ~6.1 gain */
 int32_t y=(int32_t)(((int64_t)in*gain)/256);
 /* Asymmetry: positive and negative half-cycles compress differently. */
 int32_t knee=y>=0?16500:20500;
 int32_t val=soft_ratio(y,knee==16500u?b1_clip_16500:b1_clip_20500,(uint32_t)knee);
 return (in*(int32_t)(100u-drive)+val*(int32_t)drive)/100;
}
/* Dual route: B3 path stays articulate through rotary. ROCK path stacks two
 * asymmetric drive stages and an inexpensive guitar-cabinet bandpass. All
 * controls are smoothed by one-pole states; no RT heap allocation or I/O. */
/* V3.9: exact signed 32-bit Q15 multiply with truncation toward zero.
 * The ROCK cabinet previously invoked 64-bit multiply/divide support four
 * times per sample for its one-pole filters. Split unsigned magnitude into
 * native 16-bit halves. 0 <= coefficient <= 32767, so both products and
 * their sum fit signed 32-bit for every signed 32-bit input, including MIN.
 * This is mathematically identical to ((int64_t)x*coefficient)/32768.
 * Unlike b1_mul_q15(), negative fractions round toward zero, not -infinity.
 */
static int32_t b1_filter_q15_exact(int32_t x,uint16_t coefficient){
 uint32_t mag=b1_abs32(x);
 uint32_t high=((mag>>16)*(uint32_t)coefficient)<<1;
 uint32_t low=((mag&65535u)*(uint32_t)coefficient)>>15;
 uint32_t result=high+low;
 return x<0?-(int32_t)result:(int32_t)result;
}
/* All controls remain constant within one PCM render call. Building these
 * once prevents repeated tone arithmetic 44,100 times a second. */
struct b1_block_amp {
 uint16_t tone_coefficient,treble_coefficient;
 int32_t high_weight,air_weight;
};
static struct b1_block_amp b1_prepare_amp(const struct b1_engine *e){
 struct b1_block_amp p;
 p.tone_coefficient=(uint16_t)(1250u+(uint32_t)e->amp_tone*72u);
 p.treble_coefficient=(uint16_t)(12500u+(uint32_t)e->amp_tone*45u);
 p.high_weight=5+(int32_t)e->amp_tone/20;
 p.air_weight=(int32_t)e->amp_tone/20;
 return p;
}
static int32_t b1_amp_path(struct b1_engine *e,int32_t pre,const struct b1_block_amp *p){
 /* Skip the inaudible amplifier when a mode has fully settled. During
  * transitions evaluate both paths to preserve the 25ms crossfade. */
 int32_t clean=0,rock=0;
 /* ~25ms clickless change while both paths keep their filter history. */
 uint16_t target=e->sound_mode==B1_SOUND_ROCK?32767u:0u;
 if(e->mode_mix_q15<target){uint32_t n=e->mode_mix_q15+30u;e->mode_mix_q15=(uint16_t)(n>target?target:n);}
 else if(e->mode_mix_q15>target){e->mode_mix_q15=(uint16_t)(e->mode_mix_q15>30u?e->mode_mix_q15-30u:0u);}
 if(e->mode_mix_q15<32767u)
  clean=amp_saturate(pre,(uint8_t)(e->drive/4u));
 if(e->mode_mix_q15>0u){
 e->amp_hp+=(pre-e->amp_hp)/384;
 int32_t highpassed=pre-e->amp_hp;
 /* Open up the previously dark preamp: at high TONE the upper-mid path
  * stays audible instead of always being buried 3:1 beneath low-pass audio.
  * All coefficients bounded below Nyquist, no allocations/division by state. */
 e->amp_tone_lp+=b1_filter_q15_exact(highpassed-e->amp_tone_lp,p->tone_coefficient);
 int32_t high_weight=p->high_weight; /* 5..10 */
 int32_t shaped=(highpassed*high_weight+e->amp_tone_lp*(12-high_weight))/12;
 /* A small slow power-supply sag responds to total energy/chords, but does
  * not hard-compress every onset. 3ms attack and slower release at 44.1k. */
 int32_t magnitude=highpassed>=0?highpassed:-highpassed;
 if(magnitude>16000)magnitude=16000;
 /* 128 and 4096 are powers of two. Use signed truncation-toward-zero
  * rather than variable division; signed right-shift would round negative
  * values downward and alter the amplifier's PCM output. */
 int32_t sag_diff=magnitude-e->amp_sag;
 if(sag_diff>=0)e->amp_sag+=sag_diff/128;
 else e->amp_sag+=sag_diff/4096;
 int32_t sag_q8=256-e->amp_sag/500; /* 256..224 = max 12.5% reduction */
 shaped=(int32_t)(((int64_t)shaped*sag_q8)/256);
 int32_t stage1=amp_saturate(shaped,e->amp_gain);
 int32_t stage2=amp_saturate(stage1,e->drive);
 /* Brightened speaker cabinet with an explicit, restrained mid-presence
  * hump (roughly 150Hz–1.5kHz), tight low end, and adjustable top rolloff.
  * This is original filter modelling, not a measured Marshall IR. */
 e->cab_bass+=b1_filter_q15_exact(stage2-e->cab_bass,850u);
 e->cab_presence+=b1_filter_q15_exact(stage2-e->cab_presence,7800u);
 e->cab_treble+=b1_filter_q15_exact(stage2-e->cab_treble,p->treble_coefficient);
 int32_t speaker=e->cab_treble-(e->cab_bass*7)/10+(e->cab_presence-e->cab_bass)/3;
 /* Retain some direct upper partials for bite without reverting to fizz. */
 speaker+=(stage2-e->cab_treble)*p->air_weight/16;
 rock=(int32_t)(((int64_t)stage2*(100u-e->cabinet_mix)+(int64_t)speaker*e->cabinet_mix)/100);
 }
 return (int32_t)(((int64_t)clean*(32767u-e->mode_mix_q15)+(int64_t)rock*e->mode_mix_q15)/32767);
}
/* V4.1 SMART IDLE: advance the *logical* tonewheel clock, never the wave
 * synthesizer, when the entire DSP is proven silent. The actual 91 phases
 * remain lazy and are reconstructed by shared_wheel_sample at the next
 * sounding frame. No notes, percussion, output gate, wet tails or incremental
 * delay/reverb clears may be pending when this routine is called.
 *
 * The V4.0 ramp step is done on samples 0,16,... within each render call,
 * even while silent. Advance these exact steps (including the first) so
 * changes to drawbars and oscillator mode during idle remain click-free.
 * Do NOT advance Leslie/scanner phases here: V4.0 also holds them during
 * settled silence; preserving their state makes wake-up PCM bit-identical.
 */
static void b1_advance_idle_clock(struct b1_engine *e,uint32_t frames){
 const uint16_t target=e->osc_mode==B1_OSC_SAW?32767u:0u;
 if(e->osc_blend_q15<target){
  uint32_t next=(uint32_t)e->osc_blend_q15+frames*128u;
  e->osc_blend_q15=(uint16_t)(next>target?target:next);
 }else if(e->osc_blend_q15>target){
  uint32_t step=frames*128u;
  e->osc_blend_q15=(uint16_t)(step>e->osc_blend_q15?0u:e->osc_blend_q15-step);
 }
 /* The caller declines the fast path on the extremely rare frame-counter
  * wrap: the ordinary renderer then performs its original stamp reset. */
 e->wheel_frame+=frames;
 uint32_t ramp_steps=frames/16u+((frames&15u)!=0u);
 for(uint32_t bank=0u;bank<B1_BANKS;bank++)for(uint32_t h=0u;h<B1_HARMONICS;h++){
  uint32_t v=e->pull_smooth_q16[bank][h];
  const uint32_t dest=(uint32_t)pull_q8[e->drawbars[bank][h]]<<8;
  for(uint32_t step=0u;step<ramp_steps;step++)v=ramp(v,dest,5u);
  e->pull_smooth_q16[bank][h]=v;
 }
}
void b1_render_stereo(struct b1_engine *e,int16_t *out,uint32_t frames){
 if(!out)return;
#ifndef B1_DISABLE_SMART_IDLE
 /* DMA output still receives fresh silent PCM. Never skip a pending note,
  * release, percussion capacitor, delay/reverb tail or FX-ring cleanup.
  * No heap, division per sample, audio quality reduction or new voice cap.
  * The subtraction guard also preserves V4.0 wrap handling exactly. */
 if(frames && frames<=4096u && e->wheel_frame<=UINT32_MAX-frames &&
    !e->percussion_env && !e->fx_was_audible && !e->fx_tail_samples &&
    !e->output_gate_q15 && e->fx_cleared &&
    e->fx_clear_step>=B1_DELAY_SAMPLES){
  uint8_t any_voice=0u;
  for(uint32_t j=0u;j<B1_VOICES;j++)any_voice|=e->voice[j].active;
  if(!any_voice){
   b1_advance_idle_clock(e,frames);
   for(uint32_t i=0u;i<frames;i++){out[2u*i]=0;out[2u*i+1u]=0;}
   return;
  }
 }
#endif
 /* During a genuinely silent voice block, advance only the logical clock.
  * The old stateful rotor/cabinet still drains as before. On the first
  * sounded block, resynchronize all 91 wheel phases exactly once. Full
  * polyphony uses the original fast 91-additions loop, avoiding a costly
  * per-wheel multiply/branch on every active sample. */

 /* Dense chord mixer: 12 fully keyed voices share at most 91 unique
  * tonewheels. On held notes the envelopes and contact gates have settled,
  * so calculating 12 x 9 individual harmonic sums PER SAMPLE is wasteful.
  * Aggregate their identical wheel gains once per 128-frame DMA chunk,
  * then render each used wheel at most once per sample. During attacks,
  * drawbar movement and release use the exact original per-note path.
  * This keeps normal polyphonic playing and every modulation feature. */
 uint8_t fast_mix=(uint8_t)(!e->dense_mix_test_bypass),steady_voices=0u;
 for(uint32_t b=0;b<B1_BANKS && fast_mix;b++)for(uint32_t h=0;h<B1_HARMONICS;h++){
  if(e->pull_smooth_q16[b][h] != ((uint32_t)pull_q8[e->drawbars[b][h]]<<8)){
   fast_mix=0u;break;
  }
 }
 if(fast_mix){
  for(uint32_t j=0;j<B1_VOICES;j++)if(e->voice[j].active){
   const struct b1_voice *v=&e->voice[j];
   /* No interpolation error during attack / release / key click. */
   /* With KEY CLICK=0, all contacts have settled after 12 samples,
    * so the sustained fast path can start ~0.7ms after a new chord.
    * With click enabled retain the full 130-sample transient window. */
   uint16_t contact_settled=(uint16_t)(e->click?130u:13u);
   if(v->releasing || v->envelope!=32767u || v->age<contact_settled){fast_mix=0u;break;}
   ++steady_voices;
  }
  if(steady_voices<3u)fast_mix=0u;
 }
 if(fast_mix){
  for(uint32_t w=0;w<B1_TONEWHEELS;w++){
   e->mixed_wheel_gain[w]=0;
   e->mixed_wheel_leak[w]=0u;
  }
  for(uint32_t j=0;j<B1_VOICES;j++)if(e->voice[j].active){
   const struct b1_voice *v=&e->voice[j];
   for(uint32_t h=0;h<B1_HARMONICS;h++){
    if(v->bank==0u && h==8u && e->percussion_enabled)continue;
    e->mixed_wheel_gain[v->wheel[h]]+=(int32_t)pull_q8[e->drawbars[v->bank][h]];
   }
   if(e->leakage){
    uint8_t w=v->leakage_wheel;
    ++e->mixed_wheel_leak[w];
   }
  }
  e->mixed_wheel_count=0u;
  for(uint32_t w=0;w<B1_TONEWHEELS;w++){
   if(e->mixed_wheel_gain[w] || e->mixed_wheel_leak[w])
    e->mixed_wheel_index[e->mixed_wheel_count++]=(uint8_t)w;
  }
 }
 /* Registration-aware headroom, BEFORE any clip/amp stages. All drawbar
  * footages plus the full 12-note chord are counted once per DMA block.
  * Prevent the hard square/flat-top that V2.1 produced when summing many
  * wide-open manuals then slamming the limiter. No divisions per sample. */
 uint32_t weighted=0u;
 for(uint32_t k=0;k<B1_VOICES;k++)if(e->voice[k].active){
  const uint8_t bank=e->voice[k].bank;
  for(uint32_t h=0;h<B1_HARMONICS;h++){
   if(bank==0u && h==8u && e->percussion_enabled)continue;
   weighted+=(uint32_t)pull_q8[e->drawbars[bank][h]];
  }
 }
 const uint32_t budget=3072u; /* Leave chord headroom while keeping 1-key registrations at unity. */
 uint16_t target_gain=(uint16_t)(weighted>budget ?
  ((uint32_t)32767u*budget)/weighted :32767u);
 if(target_gain<e->registration_gain_q15)
  e->registration_gain_q15=target_gain; /* instantaneous safety on chord-on */
 /* Higher output gain returns gradually; no abrupt volume jump at note-off. */
 uint32_t horn_nominal=e->rotor_fast?612000u:73000u;
 uint32_t drum_nominal=e->rotor_fast?573000u:68000u;
 uint32_t horn_pct=e->rotor_fast?(e->rotor_fast_pct?e->rotor_fast_pct:100u):
                  (e->rotor_slow_pct?e->rotor_slow_pct:100u);
 uint32_t drum_pct=e->rotor_drum_pct?e->rotor_drum_pct:100u;
 uint32_t horn_target=horn_nominal*horn_pct/100u;
 uint32_t drum_target=drum_nominal*horn_pct*drum_pct/10000u;
 const struct b1_block_fx block_fx=b1_prepare_fx(e);
 const struct b1_block_amp block_amp=b1_prepare_amp(e);
 const uint32_t scanner_type=e->cv_mode?((e->cv_mode-1u)%3u):0u;
 const int32_t scanner_depth_q8=(int32_t)(12u+scanner_type*7u)*(int32_t)e->cv_depth*256/100;
 const int32_t rotor_width_100=20+(int32_t)e->rotor_width*70/100;
 const uint32_t safe_wet=e->sound_mode==B1_SOUND_ROCK?(uint32_t)e->rotor_mix/3u:(uint32_t)e->rotor_mix;
 const int32_t safe_colour=256+(e->sound_mode==B1_SOUND_ROCK?(int32_t)e->drive:(int32_t)e->drive/6);
 const int32_t safe_amp_gain=256+(int32_t)e->amp_gain*3;
 const int32_t safe_cab_tone=(int32_t)e->amp_tone+30;
 /* Cache the UPPER key contacts once per audio sub-block. The keyboard/MIDI
  * dispatcher splits a render at each note event, so this list cannot go
  * stale inside the following per-sample loop. Avoid 12 per-sample voice
  * flag/bank tests while percussion decays; bounded stack, no heap. */
 uint8_t perc_wheels[B1_VOICES],perc_count=0u;
 if(e->percussion_enabled && e->percussion_env){
  uint8_t h=(uint8_t)(e->percussion_harmonic==2u?3u:4u);
  for(uint32_t j=0;j<B1_VOICES;j++){
   const struct b1_voice *v=&e->voice[j];
   if(v->active && !v->releasing && v->bank==0u)
    perc_wheels[perc_count++]=v->wheel[h];
  }
 }
 for(uint32_t t=0;t<frames;t++){
  /* Smooth oscillator-mode switching without resetting playing notes or
   * 91 shared wheel phases. Zero overhead in the settled ORGAN state. */
  uint16_t osc_target=e->osc_mode==B1_OSC_SAW?32767u:0u;
  if(e->osc_blend_q15<osc_target){
   uint32_t v=(uint32_t)e->osc_blend_q15+128u;
   e->osc_blend_q15=(uint16_t)(v>osc_target?osc_target:v);
  }else if(e->osc_blend_q15>osc_target){
   e->osc_blend_q15=(uint16_t)(e->osc_blend_q15>128u?e->osc_blend_q15-128u:0u);
  }
  /* A 32-bit timebase wraps after ~27 h. Wrap resets stamps only, never
   * oscillator phases or voices; the next sample is always recomputed. */
  if(++e->wheel_frame==0u){
   e->wheel_frame=1u;
   for(uint32_t i=0;i<B1_TONEWHEELS;i++)e->wheel_stamp[i]=0u;
  }
  /* Crossfade live drawbar movements to avoid clicks while holding notes. */
  if(!fast_mix && (t&15u)==0u)for(uint32_t b=0;b<B1_BANKS;b++)for(uint32_t h=0;h<9u;h++){
   uint32_t dest=(uint32_t)pull_q8[e->drawbars[b][h]]<<8;
   e->pull_smooth_q16[b][h]=ramp(e->pull_smooth_q16[b][h],dest,5u);
  }
  int32_t mix=0;
  uint8_t sounding_now=0u;
  if(fast_mix){
   /* Phase progression remains continuous, only harmonic additions are
    * grouped. Sum fits signed 32-bit: <=12*9*256*32767 ~=906M. */
   int32_t weighted_sum=0,leak_sum=0;
   for(uint32_t j=0;j<e->mixed_wheel_count;j++){
    uint8_t w=e->mixed_wheel_index[j];
    int32_t tone=shared_wheel_sample(e,w);
    weighted_sum+=tone*e->mixed_wheel_gain[w];
    leak_sum+=tone*(int32_t)e->mixed_wheel_leak[w];
   }
   mix=weighted_sum/4096;
   if(e->leakage)mix+=leak_sum*(int32_t)e->leakage/2700;
   sounding_now=1u;
   /* Age changes only for voice stealing once per chunk, not per sample. */
  }else for(uint32_t j=0;j<B1_VOICES;j++){
   struct b1_voice *v=&e->voice[j];if(!v->active)continue;
   if(v->releasing){if(v->envelope>256u)v->envelope-=256u;else{v->active=0u;continue;}}
   else if(v->envelope<32767u){uint32_t next=(uint32_t)v->envelope+1024u;v->envelope=(uint16_t)(next>32767u?32767u:next);}
   sounding_now=1u;
   int32_t sum=0;
   for(uint32_t h=0;h<9;h++){
    uint8_t pos=e->drawbars[v->bank][h];if(!pos)continue;
    if(v->bank==0u && h==8u && e->percussion_enabled)continue; /* B3 1' drawbar disconnect */
    uint8_t wheel=v->wheel[h];
    int32_t tone=shared_wheel_sample(e,wheel);
    /* Contact staggering: each of nine busbars closes on its own sample. */
    /* K3 CLICK controls both contact timing and key-transient level.
     * Important: signed noise / signed denominator. A prior unsigned `u`
     * suffix made negative noise wrap into ~+40900 samples at CLICK=1,
     * which explains the harsh 0-vs-1 toggle and persistent ringing.
     * At 0 all contacts close in sync and there is NO noise or release click;
     * the full 1..100 encoder range shapes the realistic stagger instead
     * of simply toggling a fixed clicking sound. */
    /* Calculate contact timing only during the short attack. Most normal
     * playback is CLICK=0 with settled voices: save two divisions, modulo
     * and several multiplies per harmonic per voice per sample. */
    if(v->age<38u){
     int32_t contact=(int32_t)v->age;
     if(e->click){
      uint32_t spread=((h*7u+v->note%5u)%25u);
      uint32_t curve=((uint32_t)e->click*(uint32_t)e->click+50u)/100u;
      contact-=(int32_t)((spread*curve+50u)/100u);
     }
     if(contact<=0)continue;
     if(contact<12)tone=tone*contact/12;
    }
    sum+=tone*(int32_t)(e->pull_smooth_q16[v->bank][h]>>8);
   }
   sum/=4096; /* 9 full pulls -> maximum ~18k per key before bus saturation */
   if(e->click && v->age<130u){
    v->seed=1664525u*v->seed+1013904223u;
    int32_t noise=(int32_t)((v->seed>>16)&65535u)-32768;
    sum+=(noise*(int32_t)e->click*(int32_t)(130u-v->age))/105000;
   }
   if(e->click&&v->releasing&&v->release_click_age<75u){
    v->seed=1664525u*v->seed+1013904223u;
    int32_t noise=(int32_t)((v->seed>>16)&65535u)-32768;
    sum+=(noise*(int32_t)e->click*(int32_t)(75u-v->release_click_age))/280000;
    ++v->release_click_age;
   }
   if(e->leakage){
    uint8_t w=v->leakage_wheel;
    int32_t tone=shared_wheel_sample(e,w);
    sum+=tone*(int32_t)e->leakage/2700;
   }
   if(v->age<65535u)++v->age;
   /* The original Hammond key is not velocity-sensitive. Velocity is accepted
    * on MIDI but deliberately does not alter drawbar amplitude. */
   mix+=(int32_t)(((int64_t)sum*v->envelope)>>15);
  }
  /* V3.8 HAMMOND UPPER PERCUSSION BUS: one non-retriggering envelope is
   * shared by ALL keys physically held on the UPPER manual. Previously only
   * the first note of a chord drove the percussion, making rock stabs and
   * glissandi sound unnaturally thin. 2nd=4', 3rd=2 2/3'.
   * Ignore released contacts, even while their audio fade is still active.
   * Reuse the already memoized shared tonewheels; cost is bounded to 12
   * voice checks and the voices on the upper manual only. A one-key phrase
   * is sample-for-sample identical to V3.7 (same oscillator and arithmetic).
   * No new stored patch fields or changes to the single-trigger latch. */
  if(e->percussion_enabled && e->percussion_env){
   int32_t percussion_bus=0;
   for(uint32_t j=0;j<perc_count;j++)
    percussion_bus+=shared_wheel_sample(e,perc_wheels[j]);
   /* 12 * 32767 * 32767 fits in int64_t and remains bounded, while the
    * existing registration and input guards handle large organ chords. */
   mix+=(int32_t)(((int64_t)percussion_bus*e->percussion_env)/260000u);
   /* Preserve original mono capacitor discharge and no legato retrigger. */
   uint32_t drop=(e->percussion_env+9999u)/10000u;
   e->percussion_env=e->percussion_env>drop?e->percussion_env-drop:0u;
  }
  /* True electronic Hammond keying: a free-running wheel is NEVER
   * automatically routed to the DAC. A short, bounded fade closes the output
   * after the final key/percussion release, then the filter/delay history is
   * cleared once. This prevents quiet everlasting notes from Q-format
   * filter residues, cabinet ring buffers and the low-cost DMA fallback. */
  uint8_t audible=(uint8_t)(sounding_now || (e->percussion_enabled && e->percussion_env!=0u));
  if(audible){
   e->fx_cleared=0u;e->fx_was_audible=1u;e->fx_tail_samples=0u;
   uint32_t gate=(uint32_t)e->output_gate_q15+512u;
   e->output_gate_q15=(uint16_t)(gate>32767u?32767u:gate);
  }else{
   if(e->fx_was_audible){
    e->fx_was_audible=0u;
    /* Music stops on key-up; only bounded, intentional wet FX tails remain. */
    e->fx_tail_samples=e->reverb_type&&e->reverb_mix ?
     (44100u+(uint32_t)e->reverb_decay*1323u) :
     (e->delay_mix ? 44100u : 0u);
   }
   if(e->fx_tail_samples){
    --e->fx_tail_samples;
    /* No tonewheel or dry oscillator may leak into an effect tail. */
    mix=0;
   }else{
    e->output_gate_q15=e->output_gate_q15>512u?(uint16_t)(e->output_gate_q15-512u):0u;
    if(!e->output_gate_q15){
     if(!e->fx_cleared){b1_clear_fx_tail(e);e->fx_clear_step=0u;}
     /* Clear long delay/reverb rings incrementally, NOT in a single ISR. */
     if(e->fx_clear_step<B1_DELAY_SAMPLES){
      uint32_t z=e->fx_clear_step++;
      e->delay_line[z]=0;
      if(z<B1_REVERB_A)e->rev_a[z]=0;
      if(z<B1_REVERB_B)e->rev_b[z]=0;
      if(z<B1_REVERB_C)e->rev_c[z]=0;
      if(z<B1_REVERB_DIFF)e->rev_diff[z]=0;
     }
     out[t*2u]=0;out[t*2u+1u]=0;
     continue;
    }
   }
  }
  /* Gentle input HP removes DC before drive. At 44.1k, >~12Hz. */
  e->cabinet_hp+=(mix-e->cabinet_hp)/512;
  /* Fixed-point 1/512 HP can otherwise get stuck at +/-511 forever:
   * integer division truncates to zero near silence, producing a persistent
   * DC offset and audible hard-edged ringing in the low-CPU path. On true
   * silence, finish draining that sub-quantum state. */
  if(!audible && e->cabinet_hp>-512 && e->cabinet_hp<512)e->cabinet_hp=0;
  /* Only a mixed, key-gated registration can reach the output. */
  if(e->registration_gain_q15<target_gain){
   uint32_t gain=(uint32_t)e->registration_gain_q15+256u;
   e->registration_gain_q15=(uint16_t)(gain>target_gain?target_gain:gain);
  }
  mix=b1_mul_q15(mix,e->registration_gain_q15);
  int32_t pre=b1_input_guard(e,mix-e->cabinet_hp);
  /* Emergency audio-preservation path. Real FM1 deadline overruns switch to
   * a bounded single-stage DSP route rather than permanently muting outputs.
   * MIDI, all nine drawbars, chords, key click, and leakage remain active;
   * CPU-expensive Leslie/cabinet effects are temporarily bypassed. */
  if(e->cpu_relief){
   /* The old emergency path *boosted* an already summed chord by as much as
    * 1.78x and drove the limiter continuously. Preserve musical headroom
    * instead. The per-block gain below is computed OUTSIDE the sample loop.
    * Mild ROCK colour stays available without expensive cabinet DSP. */
   uint32_t colour=(uint32_t)safe_colour;
   int32_t shaped=(pre*(int32_t)colour)/256;
   pre=shaped; /* headroom has ALREADY been applied upstream */
   /* The normal hardware boot deliberately uses SAFE DSP. Previous
    * releases bypassed ALL effects here and never left this branch, so the
    * player could turn ROTOR, C1-C3 or V1-V3 without hearing any change.
    * This path remains bounded: one scanner delay, two sine LUT lookups,
    * two small filters and inexpensive left/right amplitude modulation.
    * Full-mode DSP and its historical settings are left available. */
   if(e->sound_mode==B1_SOUND_ROCK){
    /* V3.8 ROCK/LORD: add the missing asymmetric valve transfer to the
     * default SAFE DSP route. Its previous symmetric clip rounded-off both
     * half-cycles identically; a pushed Hammond through a guitar amplifier
     * needs an uneven harmonic spectrum, not merely an EQ boost.
     * Only one soft-clip lookup is needed per sample; this retains SAFE's
     * bounded cost and ALL of the existing controls and effects. */
    int32_t amplified=(pre*safe_amp_gain)/256;
    int32_t clipped=soft_ratio(amplified,
     amplified>=0?b1_clip_16500:b1_clip_20500,
     amplified>=0?16500u:20500u);
    int32_t driven=(amplified*(int32_t)(100u-e->drive)+
     clipped*(int32_t)e->drive)/100;
    /* Cabinet with a bass roll-off, broad upper-mid presence, and a true
     * high-frequency ceiling; prevents alias-prone fizz from a bright SAW
     * while leaving ORGAN's 91 running tonewheels untouched. Every filter
     * keeps its own state and is evaluated only in ROCK mode. */
    e->cab_bass+=(driven-e->cab_bass)/32;
    e->cab_presence+=(driven-e->cab_presence)/5;
    e->cab_treble+=(driven-e->cab_treble)/3;
    int32_t mid=e->cab_presence-e->cab_bass;
    int32_t upper=e->cab_treble-e->cab_presence;
    int32_t speaker=e->cab_treble-e->cab_bass/3+
      (mid*safe_cab_tone)/170-
      (upper*(int32_t)(100u-e->amp_tone))/240;
    /* Zero CABINET still leaves DRIVE available: speaker and distortion
     * are separate controls, as on a physical amplifier and microphone. */
    pre=(driven*(int32_t)(100u-e->cabinet_mix)+
      speaker*(int32_t)e->cabinet_mix)/100;
   }
   /* Do not distort the entire CLEAN/B3 waveform before the effects. */
   int32_t sample=pre;
   /* The scanner is audible independently of rotor state. The modes are
    * Vibrato V1-3: 100% moving delay; Chorus C1-3: dry/wet 50:50.
    * Delay base is 5.8ms, with a depth-controlled variable delay. */
   sample=scanner_process(e,sample,scanner_depth_q8);
   int32_t l=sample,rr=sample;
   if(e->rotor_enabled && e->rotor_mix){
    uint32_t htarget=horn_target;
    uint32_t dtarget=drum_target;
    /* Ramp every 16 samples, not two expensive divisions per sample. */
    if((t&15u)==0u){
     e->rotor_inc=ramp(e->rotor_inc,htarget,1800u);
     e->drum_inc=ramp(e->drum_inc,dtarget,6800u);
    }
    e->rotor_phase+=e->rotor_inc;
    e->drum_phase+=e->drum_inc;
    /* Low/high bands rotate at different velocities, with independent
     * stereo pan and gain. This creates a clearly audible rotor sweep at
     * 100 MIX without the CPU cost of four Doppler delay interpolation taps. */
    e->rotor_lp1+=(sample-e->rotor_lp1)/8;
    e->rotor_lp2+=(e->rotor_lp1-e->rotor_lp2)/8;
    int32_t low=e->rotor_lp2,high=sample-low;
    uint32_t horn_index=e->rotor_phase>>21;
    int32_t horn=b1_sine[horn_index];
    int32_t drum=b1_sine[e->drum_phase>>21];
    int32_t width=rotor_width_100;
    int32_t pan_h=horn*width/32768;
    int32_t pan_d=drum*width/65536;
    int32_t rotating_l=(high*(100+pan_h)+low*(100+pan_d))/100;
    int32_t rotating_r=(high*(100-pan_h)+low*(100-pan_d))/100;
    /* Wet level also breathes, rather than a mathematically flat pan. */
    /* ROM lookup is exactly the V3.3 signed horn/1100 division,
     * but without a software integer divide twice every audio sample. */
    int32_t breath=b1_rotor_breathe[horn_index];
    rotating_l=rotating_l*(115+breath)/115;
    rotating_r=rotating_r*(115-breath)/115;
    uint32_t wet=safe_wet;
    l=(sample*(int32_t)(100u-wet)+rotating_l*(int32_t)wet)/100;
    rr=(sample*(int32_t)(100u-wet)+rotating_r*(int32_t)wet)/100;
   }
   space_process(e,&l,&rr,&block_fx);
   b1_output_guard(e,&l,&rr);
   out[t*2u]=clamp16(b1_mul_q15(l,e->output_gate_q15));
   out[t*2u+1u]=clamp16(b1_mul_q15(rr,e->output_gate_q15));
   continue;
  }
  pre=b1_amp_path(e,pre,&block_amp);
  /* Drive of 0 is genuinely clean. Scanner is before rotary, like a B3. */
  pre=scanner_process(e,pre,scanner_depth_q8);
  /* 2-pole Leslie crossover: separated ~800Hz rotor bands, cabinet roll-off.
   * Approximation of Leslie 122 rather than an IR or impulse-convolved clone. */
  e->rotor_lp1+=(pre-e->rotor_lp1)*3500/32768;
  e->rotor_lp2+=(e->rotor_lp1-e->rotor_lp2)*3500/32768;
  int32_t bass=e->rotor_lp2,treble=pre-bass;
  /* Independently accelerating treble horn / heavy bass drum. */
  uint32_t htarget=e->rotor_enabled?horn_target:0u;
  uint32_t dtarget=e->rotor_enabled?drum_target:0u;
  e->rotor_inc=ramp(e->rotor_inc,htarget,e->rotor_fast?38000u:70000u);
  e->drum_inc=ramp(e->drum_inc,dtarget,e->rotor_fast?210000u:240000u);
  e->rotor_phase+=e->rotor_inc;e->drum_phase+=e->drum_inc;
  int32_t left=pre,right=pre;
  e->rotor_line[e->rotor_write]=clamp16(treble);
  e->drum_line[e->rotor_write]=clamp16(bass);
  if(e->rotor_enabled&&e->rotor_mix){
   /* A Jon Lord-style guitar stack is largely dry/stationary. Keep optional
    * low rotary send for creative combinations rather than silently dropping
    * the user's existing rotor setting. */
   uint32_t wet=(uint32_t)(((uint32_t)e->rotor_mix*(32767u-e->mode_mix_q15)+(uint32_t)(e->rotor_mix/5u)*e->mode_mix_q15)/32767u);
   int32_t depth=(int32_t)(8u+e->rotor_width*18u/100u)*256;
   uint32_t hp=e->rotor_phase,dp=e->drum_phase;
   /* Opposed cabinet microphones see distinct Doppler, amplitude lobes and
    * early reflections; avoid the unnatural mirror-image modulation of V1.1. */
   int32_t hl=fractional_read(e->rotor_line,e->rotor_write,hp,168,depth);
   int32_t hr=fractional_read(e->rotor_line,e->rotor_write,hp+0x60000000u,173,depth);
   int32_t dl=fractional_read(e->drum_line,e->rotor_write,dp,240,depth/3);
   int32_t dr=fractional_read(e->drum_line,e->rotor_write,dp+0xa0000000u,248,depth/3);
   int32_t ph=b1_sine[hp>>21],pd=b1_sine[dp>>21];
   int32_t p_r=b1_sine[(hp+0x60000000u)>>21];
   int32_t d_r=b1_sine[(dp+0xa0000000u)>>21];
   hl=hl*(100+ph*34/32768)/100;hr=hr*(100+p_r*32/32768)/100;
   dl=dl*(100+pd*17/32768)/100;dr=dr*(100+d_r*17/32768)/100;
   /* One early wood-cabinet reflection per mic. Only the wet route gets it. */
   int32_t wall_l=e->rotor_line[(e->rotor_write-223u)&(B1_ROTARY_SAMPLES-1u)];
   int32_t wall_r=e->rotor_line[(e->rotor_write-257u)&(B1_ROTARY_SAMPLES-1u)];
   int32_t rotating_l=hl+dl+wall_l/10,rotating_r=hr+dr+wall_r/10;
   left=(pre*(int32_t)(100u-wet)+rotating_l*(int32_t)wet)/100;
   right=(pre*(int32_t)(100u-wet)+rotating_r*(int32_t)wet)/100;
  }
  e->rotor_write=(e->rotor_write+1u)&(B1_ROTARY_SAMPLES-1u);
  space_process(e,&left,&right,&block_fx);
  /* Clean linked-stereo limiter replaces always-on output saturation. */
  b1_output_guard(e,&left,&right);
  int32_t final_l=left,final_r=right;
  out[t*2u]=clamp16(b1_mul_q15(final_l,e->output_gate_q15));
  out[t*2u+1u]=clamp16(b1_mul_q15(final_r,e->output_gate_q15));
 }
 if(fast_mix)for(uint32_t j=0;j<B1_VOICES;j++)if(e->voice[j].active){
  uint32_t age=(uint32_t)e->voice[j].age+frames;
  e->voice[j].age=(uint16_t)(age>65535u?65535u:age);
 }
}
