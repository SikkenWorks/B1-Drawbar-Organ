/* Benchmark one 256-frame block in each musical state. Preparation, checks
 * and printing are outside the measured section. Also emits a regression
 * digest covering output and the full DSP state after every measured block. */
#define _POSIX_C_SOURCE 200809L
#include "b1_engine.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
static struct b1_engine model,engine;
static int16_t pcm[512];
static uint64_t hash(uint64_t h,const void *p,size_t n){
 const uint8_t *s=p;while(n--){h^=*s++;h*=UINT64_C(1099511628211);}return h;
}
static uint64_t ns(void){
 struct timespec ts;clock_gettime(CLOCK_THREAD_CPUTIME_ID,&ts);
 return (uint64_t)ts.tv_sec*1000000000u+ts.tv_nsec;
}
int main(int argc,char **argv){
 unsigned runs=argc>1?(unsigned)atoi(argv[1]):1;
 for(unsigned n=3;n<=12;n+=3)for(unsigned mode=0;mode<2;mode++)
 for(unsigned stage=0;stage<4;stage++){
  b1_reset(&model);model.cpu_relief=1;model.osc_mode=mode;
  model.osc_blend_q15=mode?32767:0;
  model.drive=81;model.click=85;model.leakage=34;model.sound_mode=B1_SOUND_ROCK;
  model.cv_mode=5;model.cv_depth=83;model.rotor_enabled=1;model.rotor_mix=87;
  model.delay_mix=70;model.reverb_type=3;model.reverb_mix=63;
  model.percussion_enabled=1;
  for(unsigned b=0;b<3;b++)for(unsigned h=0;h<9;h++)b1_drawbar(&model,b,h,8);
  for(unsigned i=0;i<n;i++)b1_note_on_tagged(&model,42+i*3,i%3,110,i+1);
  if(stage!=0)for(unsigned k=0;k<160;k++)b1_render_stereo(&model,pcm,256);
  if(stage==2)for(unsigned i=0;i<n;i++)b1_note_off_tagged(&model,i+1);
  if(stage==3){
   for(unsigned i=0;i<n;i+=2)b1_note_off_tagged(&model,i+1);
   b1_render_stereo(&model,pcm,39);
   b1_drawbar(&model,0,4,3);
   b1_note_on_tagged(&model,85,0,110,42);
  }
  uint64_t elapsed=0,digest=UINT64_C(14695981039346656037);
  for(unsigned k=0;k<runs;k++){
   engine=model;
   uint64_t start=ns();b1_render_stereo(&engine,pcm,256);elapsed+=ns()-start;
   if(!k){digest=hash(digest,pcm,sizeof(pcm));digest=hash(digest,&engine,sizeof(engine));}
  }
  printf("%u %u %u %016llx %.1f\n",n,mode,stage,(unsigned long long)digest,(double)elapsed/runs);
 }
}
