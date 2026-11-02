/* Host-only steady-organ benchmark; NOT a JieLi 60MHz measurement. */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "b1_engine.h"
static struct b1_engine e;
static int16_t pcm[512];
static volatile uint32_t sink;
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (double)t.tv_sec+(double)t.tv_nsec*1e-9;}
int main(void){
 for(unsigned n=3;n<=12;n+=3)for(unsigned leak=0;leak<=1;leak++){
  b1_reset(&e);e.cpu_relief=1;e.click=0;e.leakage=(uint8_t)(leak?20:0);
  e.percussion_enabled=0;e.cv_mode=0;e.cv_depth=0;e.rotor_enabled=0;e.rotor_mix=0;
  e.delay_mix=0;e.reverb_mix=0;e.reverb_type=0;e.sound_mode=B1_SOUND_B3;e.osc_mode=B1_OSC_ORGAN;
  for(unsigned b=0;b<3;b++)for(unsigned h=0;h<9;h++)b1_drawbar(&e,(uint8_t)b,(uint8_t)h,8);
  for(unsigned i=0;i<n;i++)b1_note_on_tagged(&e,(uint8_t)(48+i*2),(uint8_t)(i%3),100,i+1);
  for(unsigned k=0;k<3;k++)b1_render_stereo(&e,pcm,256);
  double start=now();
  for(unsigned k=0;k<3200;k++){b1_render_stereo(&e,pcm,256);sink+=(uint16_t)pcm[k&511u];}
  double ns=1e9*(now()-start)/(3200.0*256.0);
  printf("%u,%u,%.3f\n",n,leak,ns);
 }
 return 0;
}
