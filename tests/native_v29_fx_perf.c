#define _POSIX_C_SOURCE 200809L
#include "b1_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static struct b1_engine e;
static int16_t audio[256];
static volatile unsigned checksum;
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1.e9;}
int main(int argc,char **argv){
 if(argc!=3)return 2;
 const int mode=atoi(argv[1]),safe=atoi(argv[2]);
 b1_reset(&e);
 e.sound_mode=(uint8_t)mode;e.mode_mix_q15=mode?32767u:0u;e.cpu_relief=(uint8_t)safe;
 e.cv_mode=5;e.cv_depth=65;e.delay_mix=40;e.delay_time=44;e.delay_feedback=44;e.delay_spread=60;
 e.reverb_type=4;e.reverb_mix=40;e.reverb_decay=70;e.reverb_damp=50;
 e.rotor_enabled=1;e.rotor_fast=1;e.rotor_mix=60;
 for(int b=0;b<3;b++)for(int h=0;h<9;h++)b1_drawbar(&e,(uint8_t)b,(uint8_t)h,8);
 for(int n=0;n<12;n++)b1_note_on_tagged(&e,(uint8_t)(48+n),(uint8_t)(n%3),100,(unsigned)(n+1));
 for(int i=0;i<40;i++)b1_render_stereo(&e,audio,128);
 double start=now();for(int i=0;i<700;i++){
  b1_render_stereo(&e,audio,128);checksum+=(unsigned)audio[(i&127)*2];
 }
 printf("%d,%d,%.5f,%u\n",mode,safe,(now()-start)*1e6/(700*128),checksum);
 return 0;
}
