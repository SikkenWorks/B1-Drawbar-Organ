/* SPDX-License-Identifier: GPL-3.0-only. Reference PCM rendered by native B1 V2.6 C engine. */
#include "../firmware/b1_engine.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
static struct b1_engine e;static int16_t b[256];
static void w16(FILE*f,uint16_t n){fputc(n&255,f);fputc(n>>8,f);}
static void w32(FILE*f,uint32_t n){w16(f,(uint16_t)n);w16(f,(uint16_t)(n>>16));}
static void render(const char*filename,int rock,int n){
 FILE *f=fopen(filename,"wb");if(!f){perror(filename);exit(1);}b1_reset(&e);e.cpu_relief=1;e.sound_mode=(uint8_t)rock;
 e.drive=rock?65:20;e.amp_gain=rock?75:45;e.rotor_fast=1;e.rotor_mix=60;e.click=0;e.cv_mode=0;
 for(int j=0;j<3;j++)for(int h=0;h<9;h++)b1_drawbar(&e,(uint8_t)j,(uint8_t)h,8);
 for(int k=0;k<n;k++)b1_note_on(&e,(uint8_t)(48+k),0,127);
 const uint32_t samples=44100u*3u;fputs("RIFF",f);w32(f,36u+samples*4u);fputs("WAVEfmt ",f);w32(f,16);w16(f,1);w16(f,2);w32(f,44100);w32(f,176400);w16(f,4);w16(f,16);fputs("data",f);w32(f,samples*4u);
 for(uint32_t i=0;i<samples;i+=128u){if(i==128u*512u)for(int k=0;k<n;k++)b1_note_off(&e,(uint8_t)(48+k),0);
  b1_render_stereo(&e,b,128);
  for(uint32_t j=0;j<128&&i+j<samples;j++){w16(f,(uint16_t)b[2*j]);w16(f,(uint16_t)b[2*j+1]);}
 }
 fclose(f);
}
int main(int argc,char**argv){if(argc!=2)return 2;char p[400];
 snprintf(p,sizeof p,"%s/B1-V2.6-B3-FULL-DRAWBARS.wav",argv[1]);render(p,0,12);
 snprintf(p,sizeof p,"%s/B1-V2.6-ROCK-FULL-DRAWBARS.wav",argv[1]);render(p,1,12);return 0;
}
