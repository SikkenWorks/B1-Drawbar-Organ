/* SPDX-License-Identifier: GPL-3.0-only
 * B1 V2.5 safety regression: dense, full-drawbar chords in both DSP routes.
 */
#include "../firmware/b1_engine.h"
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
static struct b1_engine e;
static int16_t pcm[256];
static int absi(int x){return x<0?-x:x;}
static void run(uint8_t rock,uint8_t safe,uint8_t effects,int voices){
 b1_reset(&e);e.cpu_relief=safe;e.sound_mode=rock;
 e.drive=rock?78:30;e.amp_gain=rock?88:50;e.amp_tone=78;e.cabinet_mix=80;
 e.rotor_mix=100;e.cv_mode=effects?5:0;e.cv_depth=84;
 if(effects){e.delay_mix=65;e.delay_feedback=65;e.reverb_type=4;e.reverb_mix=65;e.reverb_decay=80;}
 for(int bank=0;bank<3;bank++)for(int h=0;h<9;h++)b1_drawbar(&e,(uint8_t)bank,(uint8_t)h,8);
 for(int k=0;k<voices;k++)b1_note_on(&e,(uint8_t)(48+k),0,127);
 int peak=0,hard=0;uint64_t sum=0;
 for(int b=0;b<128;b++){
  b1_render_stereo(&e,pcm,128);
  for(int i=0;i<256;i++){
   int a=absi(pcm[i]);if(a>peak)peak=a;
   hard+=(a>25000);sum+=(uint64_t)a;
  }
 }
 /* Audio remains audible, without hitting the target or saturating PCM. */
 assert(peak>250 && peak<24500 && sum>100000u);
 assert(!hard);
 assert(e.output_guard_gain_q15>0u && e.input_guard_gain_q15>0u);
 printf("rock=%d safe=%d effects=%d notes=%d peak=%d avg=%llu\n",rock,safe,effects,voices,peak,(unsigned long long)(sum/32768u));
}
int main(void){
 struct b1_engine n;b1_reset(&n);assert(n.rotor_fast==1 && n.rotor_mix==60 && n.click==0);
 for(int safe=0;safe<2;safe++)for(int rock=0;rock<2;rock++)for(int effects=0;effects<2;effects++){
  run((uint8_t)rock,(uint8_t)safe,(uint8_t)effects,1);
  run((uint8_t)rock,(uint8_t)safe,(uint8_t)effects,12);
 }
 puts("B1 V2.5: 16 dense-chord/FX headroom cases PASS");return 0;
}
