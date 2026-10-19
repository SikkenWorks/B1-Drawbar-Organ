/* GPL-3.0-only. Chord keys select a quality; non-chord keys trigger the chord
 * with that played pitch as root, independently of chord-key octave shifts. */
#include "b1_performance.h"
static const int8_t chord_intervals[B1_CHORD_TYPES][B1_MAX_CHORD_NOTES]={
 {0,4,7,-1,-1,-1},{0,3,7,-1,-1,-1},{0,4,7,11,-1,-1},
 {0,3,7,10,-1,-1},{0,4,7,10,-1,-1},{0,3,6,-1,-1,-1},
 {0,3,6,9,-1,-1},{0,4,8,-1,-1,-1},{0,2,7,-1,-1,-1},
 {0,5,7,-1,-1,-1},{0,4,7,14,-1,-1},{0,3,7,14,-1,-1},
 {0,3,6,10,-1,-1},{0,4,7,11,14,-1}
};
/* Pitch-class masks generated from the named 12-TET interval sets in
 * tests/scale-v18-catalog.json. Indices 0..3 stay identical to V1.7,
 * preserving existing V5 project assignments and their CRC values. */
static const uint16_t scale_masks[B1_SCALE_COUNT]={
 0xFFFu,
 0xAB5u,
 0x5ADu,
 0xAD5u,
 0x6ADu,
 0x5ABu,
 0x6B5u,
 0x56Bu,
 0x9ADu,
 0xAADu,
 0x9B5u,
 0x9B3u,
 0x5B3u,
 0x6D5u,
 0x55Bu,
 0x56Du,
 0x6ABu,
 0xB55u,
 0x5B5u,
 0x575u,
 0x295u,
 0x4A9u,
 0x29Du,
 0x4E9u,
 0x555u,
 0x6DBu,
 0xB6Du,
 0x999u,
 0xEB5u,
 0xBB5u,
 0x6BDu,
 0xDADu,
 0x9CDu,
 0x9ABu,
 0xAABu,
 0x973u,
 0x6CDu,
 0xD53u,
 0x18Du,
 0x4A3u,
 0x463u,
 0x2A5u,
 0x4A5u,
 0x18Bu,
 0x655u,
 0x4FDu,
 0x57Bu,
 0xAD9u
};
static void zero(void *ptr,uint32_t len){uint8_t *p=ptr;for(uint32_t i=0;i<len;i++)p[i]=0;}
void b1_performance_init(struct b1_performance *p){
 zero(p,sizeof(*p));p->global_octave=4; p->random_state=0x91C73E5Du;
 /* On a fresh installation every physical key must produce a note.
  * CHORD selectors deliberately do not sound alone, therefore they must not
  * silently occupy seven keys in the DEFAULT project. A performer explicitly
  * enables a CHORD range in the RANGE editor when desired. */
 p->range[0]=(struct b1_range){0,26,B1_ORGAN,0,0,0};
 p->range[1]=(struct b1_range){20,26,B1_ORGAN,2,0,0}; /* SOLO: upper seven keys */
 p->range[2]=(struct b1_range){20,26,B1_DISABLED,0,0,0};
 p->range[3]=(struct b1_range){0,0,B1_DISABLED,0,0,0};
 for(uint8_t i=20;i<27;i++)p->chord_map[i]=(uint8_t)(i-20u);
 p->guide=0u; /* Default OFF. Existing saved projects keep their setting. */
}
/* V2.7: user-facing CHORD ON/OFF. Legacy project mode 0 derives state
 * from its range layout; saved OFF explicitly suppresses chord selectors.
 * No CHORD range exists in a factory project until CHORD is enabled. */
static int effective_chord_range(const struct b1_performance *p){
 if(p->chord_mode==1u)return -1;
 for(int i=(int)B1_RANGES-1;i>=0;i--)
  if(p->range[i].type==B1_CHORD)return i;
 return -1;
}
uint8_t b1_chord_enabled(const struct b1_performance *p){return p&&effective_chord_range(p)>=0;}
uint8_t b1_set_chord_enabled(struct b1_performance *p,struct b1_engine *e,uint8_t on){
 if(!p||!e)return 0u;
 if(!on){b1_release_all(p,e);p->chord_mode=1u;return 1u;}
 if(effective_chord_range(p)>=0){p->chord_mode=2u;return 1u;}
 /* The previous explicit OFF may conceal an already-configured range. */
 for(int i=(int)B1_RANGES-1;i>=0;i--)if(p->range[i].type==B1_CHORD){
  b1_release_all(p,e);p->chord_mode=2u;return 1u;
 }
 /* Reuse an existing DISABLED slot only; NEVER overwrite ORGAN ranges. */
 int free_slot=-1;
 if(p->range[2].type==B1_DISABLED)free_slot=2;
 else for(int i=(int)B1_RANGES-1;i>=0;i--)if(p->range[i].type==B1_DISABLED){free_slot=i;break;}
 if(free_slot<0)return 0u;
 b1_release_all(p,e);
 struct b1_range *r=&p->range[free_slot];
 if(r->lo>=B1_KEYS||r->hi>=B1_KEYS||r->lo==r->hi){r->lo=20u;r->hi=26u;}
 r->type=B1_CHORD;r->attack=0u;p->chord_mode=2u;
 return 1u;
}
uint8_t b1_chord_attack(const struct b1_performance *p){
 int i=p?effective_chord_range(p):-1;return i>=0?p->range[i].attack:0u;
}
uint8_t b1_set_chord_attack(struct b1_performance *p,uint8_t value){
 int i=p?effective_chord_range(p):-1;
 if(i<0||value>100u)return 0u;
 p->range[i].attack=value;return 1u;
}
static const struct b1_range *range_for(const struct b1_performance *p,uint8_t key){
 /* Higher-numbered ranges override the base ORGAN range when they overlap.
  * This makes the factory all-keys ORGAN layout compatible with enabling a
  * CHORD modifier range later, without needing to shorten range 1 first. */
 for(int i=(int)B1_RANGES-1;i>=0;i--)if(p->range[i].type<=B1_CHORD && (p->range[i].type!=B1_CHORD||p->chord_mode!=1u) && key>=p->range[i].lo && key<=p->range[i].hi)return &p->range[i];
 return 0;
}
/* Resolve active keyboard ownership exactly as note-on routing does.
 * Merely having a quality in chord_map does not turn an ORGAN key into a
 * chord selector. Higher-numbered ranges override lower-numbered ones.
 * This query is used ONLY by the UI LED refresh (never the audio ISR). */
uint8_t b1_chord_selector_key(const struct b1_performance *p,uint8_t physical_key){
 if(!p||physical_key>=B1_KEYS||!b1_chord_enabled(p))return 0u;
 const struct b1_range *r=range_for(p,physical_key);
 return (uint8_t)(r&&r->type==B1_CHORD);
}
static uint8_t midi_key(uint8_t key,int8_t octave){int32_t n=(int32_t)B1_KEYBOARD_LOW_MIDI+(int32_t)key+(int32_t)octave*12;return (uint8_t)(n<0?0:n>127?127:n);}
uint8_t b1_note_allowed(const struct b1_performance *p,uint8_t n){
 uint8_t rel=(uint8_t)((n+12u-(p->scale_root%12u))%12u);
 return (uint8_t)((scale_masks[p->scale_type%B1_SCALE_COUNT]>>rel)&1u);
}
/* A chord-selector key still has a physical chromatic position. SCALE GUIDE
 * must display the requested musical scale, even over the CHORD range:
 * otherwise black keys in CHORD ranges all light up for C major.
 * The range octave offsets only move pitches by octaves (12 semitones).
 * GUIDE is therefore mapped to the 27-key chromatic F3..G5 keyboard,
 * independently from ORGAN/CHORD/DISABLED and from KEYLOCK. */
uint8_t b1_guide_key_allowed(const struct b1_performance *p,uint8_t key){
 if(key>=B1_KEYS)return 0u;
 return b1_note_allowed(p,(uint8_t)(B1_KEYBOARD_LOW_MIDI+key));
}
uint8_t b1_key_allowed(const struct b1_performance *p,uint8_t key){
 if(key>=B1_KEYS)return 0;
 const struct b1_range *r=range_for(p,key);
 if(!r)return 0;
 if(r->type==B1_CHORD)return 1;
 return b1_note_allowed(p,midi_key(key,(int8_t)(r->octave+(int8_t)p->global_octave-4)));
}
uint8_t b1_quantize_note(const struct b1_performance *p,uint8_t n){
 if(!p->keylock||b1_note_allowed(p,n))return n;
 for(uint8_t d=1;d<12;d++){
  if(n>=d&&b1_note_allowed(p,n-d))return (uint8_t)(n-d);
  if(n+d<128&&b1_note_allowed(p,n+d))return (uint8_t)(n+d);
 }
 return n;
}
static int b1_active_chord_key(const struct b1_performance *p){
 uint32_t newest=0;int key=-1;
 for(uint8_t k=0;k<B1_KEYS;k++){
  const struct b1_range *r=range_for(p,k);
  if(r&&r->type==B1_CHORD&&(p->pressed&(1u<<k))&&p->touch[k]>=newest){newest=p->touch[k];key=k;}
 }
 return key;
}
uint8_t b1_active_chord(const struct b1_performance *p){
 int key=b1_active_chord_key(p);return key<0?255u:(uint8_t)(p->chord_map[key]%B1_CHORD_TYPES);
}
/* Bounded xorshift PRNG: state advances per generated chord, never allocates. */
static uint32_t b1_random(struct b1_performance *p){
 uint32_t x=p->random_state;x^=x<<13;x^=x>>17;x^=x<<5;
 p->random_state=x?x:0x91C73E5Du;return p->random_state;
}
uint8_t b1_assign_chord_key(struct b1_performance *p,uint8_t physical_key,uint8_t quality){
 if(!p||physical_key>=B1_KEYS||quality>=B1_CHORD_TYPES||!b1_chord_enabled(p))return 0u;
 for(uint8_t i=0u;i<B1_RANGES;i++){
  const struct b1_range *r=&p->range[i];
  if(r==range_for(p,physical_key) && r->type==B1_CHORD && physical_key>=r->lo && physical_key<=r->hi){
   p->chord_map[physical_key]=quality;return 1u;
  }
 }
 return 0u;
}
uint8_t b1_shift_octave(struct b1_performance *p,struct b1_engine *e,int8_t delta,uint32_t held_keys){
 if(!p||!e||!delta)return 0u;
 int32_t next=(int32_t)p->global_octave+delta;
 if(next<0)next=0;
 if(next>8)next=8;
 if(next==p->global_octave)return 0u;
 b1_release_all(p,e);
 p->global_octave=(uint8_t)next;
 /* Already-held keys are immediately retuned to the new octave. */
 b1_performance_keys(p,e,held_keys);
 return 1u;
}
void b1_release_all(struct b1_performance *p,struct b1_engine *e){
 for(uint8_t k=0;k<B1_KEYS;k++){
  struct b1_held *h=&p->held[k];
  b1_note_off_tagged(e,(uint32_t)k+1u);
  h->count=0;for(uint8_t i=0;i<B1_MAX_CHORD_NOTES;i++)h->pending[i]=0u;p->touch[k]=0; p->pending_mask&=~(1u<<k);
 }
 p->pressed=0;p->order=0;p->pending_mask=0u;
}
void b1_performance_keys(struct b1_performance *p,struct b1_engine *e,uint32_t keys){
 keys&=0x07ffffffu;
 uint32_t released=p->pressed&~keys,added=keys&~p->pressed;
 for(uint8_t k=0;k<B1_KEYS;k++)if(released&(1u<<k)){
  struct b1_held *h=&p->held[k];
  b1_note_off_tagged(e,(uint32_t)k+1u);
  h->count=0;for(uint8_t i=0;i<B1_MAX_CHORD_NOTES;i++)h->pending[i]=0u;p->touch[k]=0; p->pending_mask&=~(1u<<k);
 }
 p->pressed=keys;
 for(uint8_t k=0;k<B1_KEYS;k++)if(added&(1u<<k)){
  const struct b1_range *r=range_for(p,k);
  if(r&&r->type==B1_CHORD)p->touch[k]=++p->order;
 }
 int chord_key=b1_active_chord_key(p);
 uint8_t chord=chord_key<0?255u:(uint8_t)(p->chord_map[chord_key]%B1_CHORD_TYPES);
 const struct b1_range *modifier=chord_key<0?0:range_for(p,(uint8_t)chord_key);
 uint8_t attack=modifier&&modifier->type==B1_CHORD?modifier->attack:0u;
 for(uint8_t k=0;k<B1_KEYS;k++)if(added&(1u<<k)){
  const struct b1_range *r=range_for(p,k);
  if(!r||r->type!=B1_ORGAN||r->bank>=B1_BANKS)continue;
  struct b1_held *h=&p->held[k];h->count=0;
  int8_t octave=(int8_t)(r->octave+(int8_t)p->global_octave-4);
  uint8_t root=b1_quantize_note(p,midi_key(k,octave));
  uint8_t len=(uint8_t)(chord==255u?1u:B1_MAX_CHORD_NOTES);
  for(uint8_t i=0;i<len;i++){
   int32_t n=(int32_t)root+(chord==255u?0:chord_intervals[chord][i]);
   if((chord!=255u&&chord_intervals[chord][i]<0)||n<0||n>127)continue;
   h->note[h->count]=(uint8_t)n;h->bank[h->count]=r->bank;
   h->pending[h->count]=0u;h->due[h->count]=p->clock;
   h->count++;
  }
  /* B1 V2.8: visibly effective humanized attack, rather than drawing
   * independent random samples which may accidentally all land at nearly
   * the same instant. The span is 0..~65ms, quadratic across 1..100.
   * First note is immediate; subsequent note positions are individually
   * jittered within STRATIFIED time cells and their order is shuffled.
   * This guarantees a musical spread at high settings without a fixed
   * root-first arpeggio. Every random operation occurs at NOTE-ON, never
   * in the realtime sample loop. */
  if(chord!=255u&&attack>0u&&h->count>1u){
   uint32_t window=((uint32_t)attack*(uint32_t)attack*2866u)/10000u;
   if(window<h->count-1u)window=h->count-1u;
   uint8_t order[B1_MAX_CHORD_NOTES];
   for(uint8_t i=0;i<h->count;i++)order[i]=i;
   for(uint8_t i=h->count-1u;i>0u;i--){
    uint8_t j=(uint8_t)(b1_random(p)%(i+1u));
    uint8_t tmp=order[i];order[i]=order[j];order[j]=tmp;
   }
   uint32_t last=0u;
   for(uint8_t i=0;i<h->count;i++){
    /* Random jitter contained inside each position's time cell. */
    uint32_t offset=0u;
    if(i){
     uint32_t lower=((uint32_t)i*window)/(h->count-1u);
     uint32_t upper=i==h->count-1u?window:((uint32_t)(i+1u)*window)/(h->count-1u)-1u;
     if(upper<lower)upper=lower;
     offset=lower+(b1_random(p)%(upper-lower+1u));
     if(offset<=last)offset=last+1u;
     if(offset>window)offset=window;
    }
    last=offset;
    uint8_t j=order[i];
    h->due[j]=p->clock+offset;
    h->pending[j]=offset?1u:0u;
    if(offset)p->pending_mask|=(1u<<k);
    else b1_note_on_tagged(e,h->note[j],h->bank[j],115u,(uint32_t)k+1u);
   }
  }else{
   for(uint8_t i=0;i<h->count;i++)b1_note_on_tagged(e,h->note[i],h->bank[i],115u,(uint32_t)k+1u);
  }

 }
}
/* Deliver scheduled notes at exact sample boundaries, including within an
 * FM1 128-frame DMA block. Key-up cancels future notes before rendering. */
void b1_performance_render(struct b1_performance *p,struct b1_engine *e,int16_t *out,uint32_t frames){
 /* Nearly all live blocks contain NO pending humanized chord attacks.
  * Skip 27-key scheduling scans completely and render the *same* PCM.
  * New notes/release still enter through b1_performance_keys(), and notes
  * pending inside the block take the sample-accurate path below. */
 if(!p->pending_mask){b1_render_stereo(e,out,frames);p->clock+=frames;return;}
 uint32_t done=0;
 while(done<frames){
  uint32_t next=frames-done;
  uint32_t remaining=0u;
  for(uint8_t k=0;k<B1_KEYS;k++)if(p->pending_mask&(1u<<k)){
   struct b1_held *h=&p->held[k];
   for(uint8_t i=0;i<h->count;i++)if(h->pending[i]){
    int32_t until=(int32_t)(h->due[i]-p->clock);
    if(until<=0){
     b1_note_on_tagged(e,h->note[i],h->bank[i],115u,(uint32_t)k+1u);
     h->pending[i]=0u;
    }else{
     if((uint32_t)until<next)next=(uint32_t)until;
     remaining|=(1u<<k);
    }
   }
  }
  p->pending_mask=remaining;
  /* At least one rendered sample, so this bounded loop always advances. */
  if(!next)next=1u;
  b1_render_stereo(e,out+2u*done,next);
  p->clock+=next;done+=next;
 }
}
static uint32_t project_hash(const struct b1_project *p){
 const uint8_t *x=(const uint8_t*)p;uint32_t h=2166136261u;
 for(uint32_t i=0;i<sizeof(*p)-sizeof(p->checksum);i++){h^=x[i];h*=16777619u;}
 return h;
}
int b1_project_valid(const struct b1_project *o){
 if(!o||o->magic!=0x31504242u||o->version!=6u||o->checksum!=project_hash(o))return 0;
 for(uint8_t i=0;i<B1_RANGES;i++){const struct b1_range *r=&o->ranges[i];
  if(r->type!=B1_DISABLED&&r->type!=B1_ORGAN&&r->type!=B1_CHORD)return 0;
  if(r->type!=B1_DISABLED&&(r->lo>r->hi||r->hi>=B1_KEYS||r->bank>=B1_BANKS||r->octave< -4||r->octave>4||r->attack>100u))return 0;
 }
 for(uint8_t i=0;i<B1_KEYS;i++)if(o->chords[i]>=B1_CHORD_TYPES)return 0;
 for(uint8_t b=0;b<B1_BANKS;b++)for(uint8_t h=0;h<B1_HARMONICS;h++)if(o->drawbars[b][h]>8)return 0;
 if(o->scale_root>=12||o->scale_type>=B1_SCALE_COUNT||o->keylock>1||o->guide>1||o->rotor_fast>1||o->rotor_on>1||o->percussion_on>1||(o->percussion_harmonic!=2&&o->percussion_harmonic!=3)||o->cv_mode>6u||o->cv_depth>100u||o->rotor_width>100u||o->rotor_mix>100u||o->drive>100u||o->click>100u||o->leakage>100u||o->sound_mode>B1_SOUND_ROCK||o->amp_gain>100u||o->amp_tone>100u||o->cabinet_mix>100u)return 0;
 if((o->rotor_slow_pct && (o->rotor_slow_pct<50u || o->rotor_slow_pct>150u)) ||
    (o->rotor_fast_pct && (o->rotor_fast_pct<50u || o->rotor_fast_pct>150u)) ||
    (o->rotor_drum_pct && (o->rotor_drum_pct<50u || o->rotor_drum_pct>150u)))return 0;
 if(o->delay_mix>100u||o->delay_time>100u||o->delay_feedback>90u||o->delay_spread>100u||
   o->reverb_type>4u||o->reverb_mix>100u||o->reverb_decay>100u||o->reverb_damp>100u||
   o->global_octave>8u||o->selected_bank>=B1_BANKS||o->chord_mode>2u||o->osc_mode>B1_OSC_SAW)return 0;
 return 1;
}
void b1_project_capture(struct b1_project *o,const struct b1_performance *p,const struct b1_engine *e){
 zero(o,sizeof(*o));o->magic=0x31504242u;o->version=6u;
 for(uint8_t b=0;b<B1_BANKS;b++)for(uint8_t h=0;h<B1_HARMONICS;h++)o->drawbars[b][h]=e->drawbars[b][h];
 for(uint8_t i=0;i<B1_RANGES;i++)o->ranges[i]=p->range[i];
 for(uint8_t i=0;i<B1_KEYS;i++)o->chords[i]=p->chord_map[i];
 o->scale_root=p->scale_root;o->scale_type=p->scale_type;o->keylock=p->keylock;o->guide=p->guide;
 o->rotor_fast=e->rotor_fast;o->rotor_on=e->rotor_enabled;o->drive=e->drive;o->click=e->click;
 o->leakage=e->leakage;o->percussion_on=e->percussion_enabled;o->percussion_harmonic=e->percussion_harmonic;
 o->cv_mode=e->cv_mode;o->cv_depth=e->cv_depth;o->rotor_width=e->rotor_width;o->rotor_mix=e->rotor_mix;
 o->sound_mode=e->sound_mode;o->amp_gain=e->amp_gain;o->amp_tone=e->amp_tone;o->cabinet_mix=e->cabinet_mix;
 o->rotor_slow_pct=e->rotor_slow_pct;o->rotor_fast_pct=e->rotor_fast_pct;o->rotor_drum_pct=e->rotor_drum_pct;
 o->delay_mix=e->delay_mix;o->delay_time=e->delay_time;o->delay_feedback=e->delay_feedback;o->delay_spread=e->delay_spread;
 o->reverb_type=e->reverb_type;o->reverb_mix=e->reverb_mix;o->reverb_decay=e->reverb_decay;o->reverb_damp=e->reverb_damp;
 o->global_octave=p->global_octave;o->selected_bank=p->selected_bank;
 o->chord_mode=(p->chord_mode==1u?1u:2u);
 o->osc_mode=e->osc_mode;
 o->checksum=project_hash(o);
}
int b1_project_apply(const struct b1_project *o,struct b1_performance *p,struct b1_engine *e){
 if(!b1_project_valid(o))return 0;
 b1_release_all(p,e);b1_panic(e);
 for(uint8_t b=0;b<B1_BANKS;b++)for(uint8_t h=0;h<B1_HARMONICS;h++)e->drawbars[b][h]=o->drawbars[b][h];
 for(uint8_t i=0;i<B1_RANGES;i++)p->range[i]=o->ranges[i];
 for(uint8_t i=0;i<B1_KEYS;i++)p->chord_map[i]=o->chords[i];
 p->scale_root=o->scale_root;p->scale_type=o->scale_type;p->keylock=o->keylock;p->guide=o->guide;
 e->rotor_fast=o->rotor_fast;e->rotor_enabled=o->rotor_on;e->drive=o->drive;e->click=o->click;
 e->leakage=o->leakage;e->percussion_enabled=o->percussion_on;e->percussion_harmonic=o->percussion_harmonic;
 e->cv_mode=o->cv_mode;e->cv_depth=o->cv_depth;e->rotor_width=o->rotor_width;e->rotor_mix=o->rotor_mix;
 e->sound_mode=o->sound_mode;e->amp_gain=o->amp_gain;e->amp_tone=o->amp_tone;e->cabinet_mix=o->cabinet_mix;
 e->rotor_slow_pct=o->rotor_slow_pct;e->rotor_fast_pct=o->rotor_fast_pct;e->rotor_drum_pct=o->rotor_drum_pct;
 e->delay_mix=o->delay_mix;e->delay_time=o->delay_time;e->delay_feedback=o->delay_feedback;e->delay_spread=o->delay_spread;
 e->reverb_type=o->reverb_type;e->reverb_mix=o->reverb_mix;e->reverb_decay=o->reverb_decay;e->reverb_damp=o->reverb_damp;
 p->global_octave=o->global_octave;p->selected_bank=o->selected_bank;
 p->chord_mode=o->chord_mode; /* 0 means legacy auto-detect; old CRC stays valid. */
 e->osc_mode=o->osc_mode; /* blend smoothly; do not reset continuous wheel phases. */
 e->amp_hp=0;e->amp_tone_lp=0;e->cab_bass=0;e->cab_treble=0;
 return 1;
}
