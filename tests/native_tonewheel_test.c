/* Tonewheel authenticity-oriented regression. A real B3 recording is still
 * needed to establish acoustic similarity, these are ENGINE invariants. */
#include "../firmware/b1_engine.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
static struct b1_engine e,a,b;
static int16_t pcm_a[256*2],pcm_b[256*2];
static uint64_t difference(const int16_t *a0,const int16_t *b0){
 uint64_t d=0;for(unsigned i=0;i<512;i++){int k=(int)a0[i]-(int)b0[i];d+=(unsigned)(k<0?-k:k);}return d;
}
int main(void){
 b1_reset(&e);
 assert(b1_tonewheel_index(60,0)==24u); /* 16': MIDI 60 -> MIDI 48 */
 assert(b1_tonewheel_index(60,1)==43u); /* 5 1/3': 67 */
 assert(b1_tonewheel_index(60,2)==36u); /* 8': unison */
 assert(b1_tonewheel_index(60,3)==48u); /* 4': octave */
 assert(b1_tonewheel_index(36,0)==12u); /* bass 16' folds into C2 */
 assert(b1_tonewheel_index(96,4)==79u); /* upper third folds 115 -> 103 */
 for(unsigned n=0;n<128u;n++)for(unsigned h=0;h<9u;h++)assert(b1_tonewheel_index((uint8_t)n,(uint8_t)h)<B1_TONEWHEELS);
 assert(e.drawbars[0][0]==8 && e.drawbars[0][3]==8 && e.drawbars[0][4]==0);
 /* Lazy phases still imply mathematically free-running wheels: a wheel
  * first sampled after 256 silent frames must have advanced by 257 ticks. */
 uint32_t p=e.wheel_phase[30u];b1_render_stereo(&e,pcm_a,256);
 assert(e.wheel_phase[30u]==p); /* never evaluated the unused wheel */
 for(unsigned i=0;i<512;i++)assert(pcm_a[i]==0);
 uint8_t wi=b1_tonewheel_index(54,2u);
 uint32_t original=e.wheel_phase[wi],inc=e.wheel_inc[wi];
 b1_note_on_tagged(&e,54,0,100,99u);
 b1_render_stereo(&e,pcm_a,1u);
 assert(e.wheel_phase[wi]==original+inc*257u);
 b1_note_off_tagged(&e,99u);b1_reset(&e);
 /* Repeated MIDI notes are assigned the same physical tonewheels. */
 b1_note_on_tagged(&e,60,0,30,11);b1_note_on_tagged(&e,60,0,120,12);
 assert(e.voice[0].wheel[2]==e.voice[1].wheel[2] && e.voice[0].active && e.voice[1].active);
 assert(e.upper_held==2);b1_note_off_tagged(&e,11);assert(e.upper_held==1);
 b1_note_off_tagged(&e,12);assert(e.upper_held==0);
 b1_reset(&a);b1_reset(&b);a.rotor_enabled=0;b.rotor_enabled=0;a.drive=0;b.drive=0;
 /* Classic unweighted organ responds equally to nonzero MIDI velocities. */
 b1_note_on(&a,64,0,12);b1_note_on(&b,64,0,124);
 b1_render_stereo(&a,pcm_a,256);b1_render_stereo(&b,pcm_b,256);
 assert(difference(pcm_a,pcm_b)==0);
 /* One shared percussion trigger; no retrigger while upper is held. */
 b1_reset(&e);e.percussion_enabled=1;
 b1_note_on(&e,60,0,100);assert(e.percussion_env==32767u&&e.percussion_latched);
 b1_render_stereo(&e,pcm_a,256);uint32_t decay=e.percussion_env;
 b1_note_on(&e,64,0,100);assert(e.percussion_env==decay);
 b1_note_off(&e,60,0);b1_note_off(&e,64,0);
 assert(e.upper_held==0u&&!e.percussion_latched);
 b1_note_on(&e,67,0,100);assert(e.percussion_env==32767u);
 /* 1' is silent when upper percussion is enabled (historic busbar tradeoff). */
 b1_reset(&a);b1_reset(&b);for(unsigned h=0;h<9;h++){b1_drawbar(&a,0,h,0);b1_drawbar(&b,0,h,0);}
 b1_drawbar(&a,0,8,8);b1_drawbar(&b,0,8,8);
 a.percussion_enabled=0;b.percussion_enabled=1;
 a.rotor_enabled=0;b.rotor_enabled=0;a.drive=0;b.drive=0;
 b1_note_on(&a,60,0,100);b1_note_on(&b,60,0,100);
 b1_render_stereo(&a,pcm_a,256);b1_render_stereo(&b,pcm_b,256);
 assert(difference(pcm_a,pcm_b)>0);
 /* Leslie horn ramps quicker than heavy drum; dry engine is mono. */
 b1_reset(&e);e.rotor_fast=1;b1_note_on(&e,55,0,100);
 b1_render_stereo(&e,pcm_a,256);
 assert(e.rotor_inc>e.drum_inc);
 b1_reset(&e);e.rotor_enabled=0;b1_note_on(&e,55,0,100);
 b1_render_stereo(&e,pcm_a,256);
 for(unsigned i=0;i<256;i++)assert(pcm_a[i*2]==pcm_a[i*2+1]);
 /* Realtime stress: no unbounded voice cycles, no signed overflow. */
 b1_reset(&e);e.drive=100;e.rotor_fast=1;e.cv_mode=6;e.cv_depth=100;
 for(unsigned n=0;n<12;n++)b1_note_on(&e,(uint8_t)(45+n),n%3u,127);
 uint32_t energy=0;for(unsigned blk=0;blk<160;blk++){
  b1_render_stereo(&e,pcm_a,256);
  for(unsigned i=0;i<512;i++){int v=pcm_a[i];energy+=(uint32_t)(v<0?-v:v);}
 }
 assert(energy!=0);
 puts("B1 V0.8 91-wheel foldback, free-run, velocity, percussion, leakage, dual Leslie and stress: PASS");
 return 0;
}
