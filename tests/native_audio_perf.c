/* Software-only host benchmark of the B1 audio callback; NEVER a 60MHz FM1 timing measurement.
 * Compiles against both v1.0 and v1.1 source to compare identical scenarios. */
#define _POSIX_C_SOURCE 200809L
#include "b1_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
static struct b1_engine eng;
static int16_t pcm[128*2];
static volatile uint32_t checksum;
static double seconds(void){struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return (double)ts.tv_sec+(double)ts.tv_nsec/1e9;}
int main(void){
 const unsigned frames=128, blocks=700;
 for(int mode=0;mode<2;mode++)for(int count=0;count<=12;count+=count==0?4:8){
  b1_reset(&eng);eng.sound_mode=(uint8_t)mode;eng.mode_mix_q15=(uint16_t)(mode?32767:0);
  for(int bank=0;bank<3;bank++)for(int bar=0;bar<9;bar++)b1_drawbar(&eng,(uint8_t)bank,(uint8_t)bar,8);
  eng.cv_mode=5;eng.rotor_fast=1;
  for(int v=0;v<count;v++)b1_note_on_tagged(&eng,(uint8_t)(45+v),(uint8_t)(v%3),112,(uint32_t)v+1);
  for(int warm=0;warm<40;warm++)b1_render_stereo(&eng,pcm,frames);
  double start=seconds();for(unsigned i=0;i<blocks;i++){
   b1_render_stereo(&eng,pcm,frames);
   checksum+=(uint16_t)pcm[(i&127u)*2u];
  }
  double elapsed=seconds()-start;
  printf("%s,%2d,%.3f,%.3f\n",mode?"ROCK":"B3",count,1.e6*elapsed/(frames*blocks),100*elapsed/(frames*(double)blocks/44100.));
 }
 fprintf(stderr,"sink=%" PRIu32 "\n",checksum);return 0;
}
