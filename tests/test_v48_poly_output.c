/* Host-side PCM stress; no FM1 DMA cycle timing is measured here. */
#include "b1_engine.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
static struct b1_engine e;
static int16_t out[512];
static int32_t mag(int x){return x<0?-x:x;}
int main(void){
 unsigned run=0;
 for(int voices=1;voices<=12;voices++)for(int cfg=0;cfg<6;cfg++){
  b1_reset(&e);e.cpu_relief=(uint8_t)(cfg&1);e.osc_mode=(uint8_t)((cfg>>1)&1);
  e.sound_mode=(uint8_t)((cfg>>2)&1);e.click=70;e.leakage=25;
  e.rotor_enabled=1;e.rotor_mix=65;e.percussion_enabled=1;e.percussion_harmonic=3;
  e.cv_mode=3;e.cv_depth=52;e.delay_mix=cfg&1?35:0;
  e.reverb_type=cfg&1?3:0;e.reverb_mix=cfg&1?35:0;
  for(int b=0;b<3;b++)for(int h=0;h<9;h++)b1_drawbar(&e,b,h,8u);
  for(int j=0;j<voices;j++)b1_note_on(&e,(uint8_t)(44+j*3),(uint8_t)(j%3),120);
  int clipped=0;int32_t peak=0;
  for(int k=0;k<40;k++){
   b1_render_stereo(&e,out,256);
   for(int i=0;i<512;i++){
    int32_t a=mag(out[i]);if(a>peak)peak=a;
    if(a>=32767)clipped++;
   }
  }
  if(clipped){fprintf(stderr,"FAIL voices %d cfg %d peak %d clipped %d\n",voices,cfg,peak,clipped);return 1;}
  ++run;
 }
 printf("PASS %u 256-frame polyphony/FX output stress cases: no hard 16-bit clipping (host only).\n",run);
 return 0;
}
