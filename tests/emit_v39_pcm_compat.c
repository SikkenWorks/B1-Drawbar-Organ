/* GPL-3.0-only. Generate deterministic B1 PCM from both base and proposed
 * engine. Compare the files byte for byte; no new instrumentation in DSP. */
#include "b1_engine.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static struct b1_engine e;
static int16_t pcm[256];
static uint32_t rnd=0xDEADBEEFu;
static uint32_t random32(void){ rnd^=rnd<<13; rnd^=rnd>>17; rnd^=rnd<<5;return rnd; }
int main(void){
 for(unsigned scenario=0; scenario<16; ++scenario){
  b1_reset(&e);
  e.cpu_relief=(uint8_t)((scenario>>3)&1u);
  e.sound_mode=(uint8_t)((scenario&1u)?B1_SOUND_ROCK:B1_SOUND_B3);
  e.mode_mix_q15=(uint16_t)(e.sound_mode==B1_SOUND_ROCK?32767:0);
  e.osc_mode=(uint8_t)(((scenario>>1)&1u)?B1_OSC_SAW:B1_OSC_ORGAN);
  e.rotor_enabled=(uint8_t)((scenario>>2)&1u);
  e.rotor_mix=72u;
  e.click=18u;e.leakage=21u;e.percussion_enabled=1u;
  e.cv_mode=5u;e.cv_depth=48u;e.delay_mix=35u;e.reverb_type=3u;
  e.reverb_mix=27u;e.reverb_decay=68u;e.reverb_damp=57u;
  e.drive=67u;e.amp_gain=71u;e.amp_tone=76u;e.cabinet_mix=88u;
  e.dense_mix_test_bypass=(uint8_t)((scenario&1u)!=0u);
  for(unsigned block=0;block<128u;++block){
   if(block==0u){for(unsigned n=0;n<12u;n++)b1_note_on(&e,(uint8_t)(48u+n*3u),(uint8_t)(n%3u),100u);}
   if(block==19u || block==82u){e.amp_tone=(uint8_t)(random32()%101u);e.amp_gain=(uint8_t)(random32()%101u);e.cabinet_mix=(uint8_t)(random32()%101u);e.drive=(uint8_t)(random32()%101u);}
   if(block==42u){e.sound_mode=(uint8_t)(e.sound_mode==B1_SOUND_B3?B1_SOUND_ROCK:B1_SOUND_B3);}
   if(block==55u){e.osc_mode=(uint8_t)(e.osc_mode==B1_OSC_SAW?B1_OSC_ORGAN:B1_OSC_SAW);}
   if(block==62u){for(unsigned n=0;n<8u;n++)b1_note_off(&e,(uint8_t)(48u+n*3u),(uint8_t)(n%3u));}
   if(block==97u){for(unsigned n=8u;n<12u;n++)b1_note_off(&e,(uint8_t)(48u+n*3u),(uint8_t)(n%3u));}
   b1_render_stereo(&e,pcm,128u);
   if(fwrite(pcm,sizeof(pcm),1u,stdout)!=1u)return 1;
  }
 }
 return 0;
}
