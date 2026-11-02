/* x86 host-only comparison, not target timing. No output interpretation as FM1 CPU. */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "b1_engine.h"
static struct b1_engine e;
static int16_t pcm[512];
static volatile uint32_t sink;
static double seconds(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+(double)t.tv_nsec*1e-9;}
int main(void){
 for(int safe=0;safe<=1;safe++)for(int rock=0;rock<=1;rock++){
  b1_reset(&e);e.cpu_relief=(uint8_t)safe;
  e.sound_mode=(uint8_t)rock;e.mode_mix_q15=rock?32767:0;
  e.drive=rock?85:20;e.amp_gain=70;e.amp_tone=70;e.cabinet_mix=80;
  e.rotor_enabled=1;e.rotor_mix=75;e.rotor_width=80;e.rotor_fast=1;
  e.cv_mode=5;e.cv_depth=55;
  e.reverb_type=3;e.reverb_mix=55;e.reverb_decay=75;e.reverb_damp=45;
  e.delay_mix=25;e.delay_time=35;e.delay_feedback=40;
  e.leakage=20;e.click=50;e.percussion_enabled=1;
  for(unsigned bank=0;bank<3;bank++)for(unsigned h=0;h<9;h++)b1_drawbar(&e,(uint8_t)bank,(uint8_t)h,8);
  for(unsigned v=0;v<12;v++)b1_note_on_tagged(&e,(uint8_t)(46+v*2),(uint8_t)(v%3),100,v+1);
  for(int k=0;k<5;k++)b1_render_stereo(&e,pcm,256);
  double t0=seconds();
  for(unsigned k=0;k<1500;k++){b1_render_stereo(&e,pcm,256);sink+=pcm[(k&255)*2];}
  printf("safe=%d rock=%d ns/sample=%.2f\n",safe,rock,(seconds()-t0)*1e9/(1500.0*256.0));
 }
 return (int)(sink == UINT32_MAX);
}
