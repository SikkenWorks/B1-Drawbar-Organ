/* Repeatable host-only stress benchmark; not equivalent to JieLi execution time. */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include "b1_engine.h"
static struct b1_engine e;
static int16_t pcm[512];
static volatile int sink;
static double secs(void){struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return ts.tv_sec+ts.tv_nsec*1e-9;}
int main(void){
 for(unsigned count=3;count<=12;count+=3)for(unsigned rock=0;rock<=1;rock++){
  double elapsed=0.;uint32_t sample_count=0;
  for(unsigned rep=0;rep<110;rep++){
   b1_reset(&e);
   e.cpu_relief=0;
   e.sound_mode=(uint8_t)rock;
   e.mode_mix_q15=(uint16_t)(rock?32767:0);
   e.click=75;e.leakage=46;e.percussion_enabled=1;
   e.cv_mode=4;e.cv_depth=42;e.rotor_mix=60;
   e.delay_mix=37;e.reverb_mix=35;e.reverb_type=2;
   for(unsigned bank=0;bank<3;bank++)for(unsigned h=0;h<9;h++)b1_drawbar(&e,(uint8_t)bank,(uint8_t)h,(uint8_t)((h+bank)%3?8:0));
   for(unsigned v=0;v<count;v++)b1_note_on_tagged(&e,(uint8_t)(46+v*2),(uint8_t)(v%3),100,v+1);
   double start=secs();
   for(unsigned k=0;k<4;k++){b1_render_stereo(&e,pcm,256);sink+=pcm[k&3];}
   elapsed+=secs()-start;sample_count+=1024;
  }
  printf("%u,%s,%.3f\n",count,rock?"ROCK":"B3",1e9*elapsed/sample_count);
 }
 return 0;
}
