/* SPDX-License-Identifier: GPL-3.0-only. 12 voices, maximum modulation, full safe DSP, bounded PCM. */
#include "../firmware/b1_engine.h"
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
static struct b1_engine e;
static int16_t pcm[256];
int main(void){
 for(int safe=0;safe<=1;safe++){
  b1_reset(&e);e.cpu_relief=(uint8_t)safe;
  e.sound_mode=B1_SOUND_ROCK;e.drive=78u;e.amp_gain=65u;e.rotor_enabled=1u;e.rotor_fast=1u;
  e.cv_mode=6u;e.cv_depth=100u;e.rotor_mix=100u;
  e.delay_mix=75u;e.delay_time=90u;e.delay_feedback=85u;e.delay_spread=100u;
  e.reverb_type=4u;e.reverb_mix=90u;e.reverb_decay=98u;e.reverb_damp=100u;
  for(uint8_t b=0;b<3;b++)for(uint8_t i=0;i<9;i++)b1_drawbar(&e,b,i,8u);
  for(uint8_t i=0;i<12;i++)b1_note_on_tagged(&e,(uint8_t)(48u+i),(uint8_t)(i%3u),120u,(uint32_t)i+1u);
  uint32_t peaks=0u;int64_t total=0;
  for(uint32_t block=0;block<1000u;block++){
   b1_render_stereo(&e,pcm,128u);
   for(uint32_t i=0;i<256u;i++){
    int32_t a=pcm[i];if(a<0)a=-a;
    assert(a<=32767);if((uint32_t)a>peaks)peaks=(uint32_t)a;
    total+=a;
   }
  }
  assert(total>10000000&&peaks>100);
  printf("max FX DSP=%d 12 voices all drawbars: peak=%u absolute=%lld PASS\n",safe,peaks,(long long)total);
 }
 return 0;
}
