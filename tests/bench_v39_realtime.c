/* GPL-3.0-only. Host-only throughput: NOT FM1 60MHz processor timing. */
#define _POSIX_C_SOURCE 200809L
#include "b1_engine.h"
#include <time.h>
#include <stdio.h>
#include <stdint.h>
static struct b1_engine e;
static int16_t out[256];
static double now(void){struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return ts.tv_sec+1e-9*ts.tv_nsec;}
int main(void){
 for(unsigned relief=0;relief<2;relief++){
  for(unsigned n=0;n<2;n++){
   b1_reset(&e);e.cpu_relief=relief;e.sound_mode=B1_SOUND_ROCK;e.mode_mix_q15=32767;
   e.amp_tone=75;e.drive=80;e.amp_gain=80;e.cabinet_mix=90;
   e.rotor_enabled=0;e.rotor_mix=0;e.delay_mix=0;e.reverb_mix=0;e.reverb_type=0;
   e.click=0;e.percussion_enabled=0;e.leakage=0;
   for(unsigned i=0;i<(n?10:3);i++)b1_note_on(&e,(uint8_t)(48+i*3),(uint8_t)(i%3),100);
   uint64_t checksum=0;
   double begin=now();
   for(unsigned k=0;k<4000;k++){
    b1_render_stereo(&e,out,128);
    checksum+=(uint16_t)out[k&255u];
   }
   double sec=now()-begin;
   printf("DSP=%s VOICES=%u seconds=%.6f us_per_sample=%.4f chk=%llu\n",
    relief?"SAFE":"FULL",n?10:3,sec,sec*1e6/(4000.0*128.0),(unsigned long long)checksum);
  }
 }
 return 0;
}
