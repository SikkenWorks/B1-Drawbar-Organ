/* SPDX-License-Identifier: GPL-3.0-only - V3.3 saw mode stress regression. */
#include "../firmware/b1_engine.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static struct b1_engine e;
static int16_t audio[2*128];
int main(void){
 for(int relief=0;relief<=1;relief++)for(int mode=0;mode<=1;mode++)for(int rock=0;rock<=1;rock++)for(int wet=0;wet<=1;wet++){
  b1_reset(&e);e.cpu_relief=(uint8_t)relief;e.osc_mode=(uint8_t)mode;
  e.sound_mode=(uint8_t)rock;e.mode_mix_q15=rock?32767u:0;
  if(wet){e.cv_mode=5;e.cv_depth=95;e.delay_mix=80;e.delay_feedback=80;e.reverb_type=4;e.reverb_mix=80;e.reverb_decay=85;}
  for(int b=0;b<3;b++)for(int j=0;j<9;j++)b1_drawbar(&e,(uint8_t)b,(uint8_t)j,8u);
  for(int v=0;v<12;v++)b1_note_on_tagged(&e,(uint8_t)(52+v*2),(uint8_t)(v%3),125u,(uint32_t)v+1u);
  int peak=0;long long total=0;
  for(int block=0;block<100;block++){
   b1_render_stereo(&e,audio,128u);
   for(unsigned k=0;k<256;k++){
    int val=audio[k];if(val<0)val=-val;
    if(val>peak)peak=val;
    total+=val;
   }
  }
  if(peak>=32760||total<10000){fprintf(stderr,"V3.3 stress failure relief=%d mode=%d rock=%d fx=%d peak=%d total=%lld\n",relief,mode,rock,wet,peak,total);return 1;}
  if((mode && e.osc_blend_q15!=32767u)||(!mode && e.osc_blend_q15!=0u)){fprintf(stderr,"mode did not settle\n");return 1;}
  printf("relief=%d wave=%d rock=%d FX=%d peak=%d\n",relief,mode,rock,wet,peak);
 }
 puts("B1 V3.3: 16 dense-chord, full-register, full FX combinations PASS");
 return 0;
}
