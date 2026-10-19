/* SPDX-License-Identifier: GPL-3.0-only
 * V3.8 upper-manual single-trigger percussion and ROCK DSP smoke tests.
 * Runs native; does not claim subjective listening or physical hardware validation.
 */
#include "../firmware/b1_engine.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static struct b1_engine one,two,lower,rock_clean,rock_driven,organ;
static int16_t oa[256],ob[256],oc[256],od[256];
static void prep(struct b1_engine *e){
 b1_reset(e);e->cpu_relief=1u;
 e->rotor_enabled=0u;e->rotor_mix=0u;e->drive=0u;e->click=0u;e->leakage=0u;
 e->cv_mode=0u;e->delay_mix=0u;e->reverb_mix=0u;e->reverb_type=0u;
 e->percussion_enabled=1u;e->percussion_harmonic=3u;
 for(unsigned b=0;b<3;b++)for(unsigned h=0;h<9;h++)b1_drawbar(e,b,h,0u);
}
static uint64_t difference(const int16_t *a,const int16_t *b){
 uint64_t d=0u;for(unsigned i=0;i<256;i++){int32_t x=(int32_t)a[i]-(int32_t)b[i];d+=(uint32_t)(x<0?-x:x);}return d;
}
int main(void){
 prep(&one);prep(&two);prep(&lower);
 b1_note_on_tagged(&one,60u,0u,100u,1u);
 b1_note_on_tagged(&two,60u,0u,100u,1u);
 b1_note_on_tagged(&lower,60u,0u,100u,1u);
 /* Align free-running wheel phases and the single capacitor envelope. */
 for(unsigned k=0;k<8;k++){
  b1_render_stereo(&one,oa,128u);
  b1_render_stereo(&two,ob,128u);
  b1_render_stereo(&lower,oc,128u);
  assert(!memcmp(oa,ob,sizeof(oa)) && !memcmp(oa,oc,sizeof(oa)));
 }
 unsigned before=two.percussion_env;
 b1_note_on_tagged(&two,67u,0u,100u,2u);
 b1_note_on_tagged(&lower,67u,1u,100u,2u);
 /* A new upper key must not retrigger the shared percussion envelope. */
 assert(two.percussion_env==before && two.percussion_latched==1u);
 b1_render_stereo(&one,oa,128u);
 b1_render_stereo(&two,ob,128u);
 b1_render_stereo(&lower,oc,128u);
 assert(difference(oa,ob)>100u);  /* Upper E? actually G adds tone. */
 assert(difference(oa,oc)==0u);    /* Lower manual does not feed percussion. */
 assert(two.percussion_env==one.percussion_env);
 b1_note_off_tagged(&two,1u);
 assert(two.percussion_latched==1u && two.percussion_env!=0u);
 b1_note_off_tagged(&two,2u);
 assert(two.percussion_latched==0u && two.percussion_env==0u);
 puts("PASS: mono envelope, all held upper notes, single trigger, lower separation, release");
 /* ROCK SAFE path remains controllable and strongly differs from B3.
  * Wet FX, stereo and the SAW oscillator have independent older tests. */
 prep(&rock_clean);prep(&rock_driven);prep(&organ);
 rock_clean.percussion_enabled=rock_driven.percussion_enabled=organ.percussion_enabled=0u;
 rock_clean.sound_mode=rock_driven.sound_mode=B1_SOUND_ROCK;
 rock_clean.mode_mix_q15=rock_driven.mode_mix_q15=32767u;
 rock_clean.drive=0u;rock_clean.amp_gain=45u;rock_clean.cabinet_mix=0u;
 rock_driven.drive=85u;rock_driven.amp_gain=78u;rock_driven.amp_tone=65u;rock_driven.cabinet_mix=80u;
 organ.sound_mode=B1_SOUND_B3;organ.mode_mix_q15=0u;organ.drive=0u;
 for(unsigned h=0;h<9;h++){
  b1_drawbar(&rock_clean,0,h,h<4?8u:0u);
  b1_drawbar(&rock_driven,0,h,h<4?8u:0u);
  b1_drawbar(&organ,0,h,h<4?8u:0u);
 }
 b1_note_on(&rock_clean,60u,0u,100u);
 b1_note_on(&rock_driven,60u,0u,100u);
 b1_note_on(&organ,60u,0u,100u);
 uint64_t delta_rock=0u,delta_b3=0u;unsigned peak=0u;
 for(unsigned k=0;k<96;k++){
  b1_render_stereo(&rock_clean,oa,128u);
  b1_render_stereo(&rock_driven,ob,128u);
  b1_render_stereo(&organ,od,128u);
  if(k>4u){delta_rock+=difference(oa,ob);delta_b3+=difference(ob,od);}
  for(unsigned j=0;j<256;j++){
   int32_t a=ob[j];unsigned p=(unsigned)(a<0?-a:a);if(p>peak)peak=p;
  }
 }
 assert(delta_rock>50000u && delta_b3>50000u);
 assert(peak<30000u);
 printf("PASS: safe ROCK responds to GAIN/DRIVE/CABINET, distinct from B3, peak=%u\n",peak);
 return 0;
}
