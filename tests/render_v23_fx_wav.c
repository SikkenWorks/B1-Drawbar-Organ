/* SPDX-License-Identifier: GPL-3.0-only. Render native FM1 DSP WAV references. */
#include "../firmware/b1_engine.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static struct b1_engine eng;
static int16_t pcm[256];
static void w16(FILE *f,uint16_t a){fputc(a&255u,f);fputc(a>>8,f);}
static void w32(FILE *f,uint32_t a){w16(f,(uint16_t)a);w16(f,(uint16_t)(a>>16));}
static void one(const char *file,int variant){
 FILE *f=fopen(file,"wb");if(!f){perror(file);exit(1);}
 b1_reset(&eng);eng.cpu_relief=1u;eng.click=0u;eng.leakage=3u;eng.rotor_enabled=0u;
 if(variant==1){eng.cv_mode=5;eng.cv_depth=82;}
 if(variant==2){eng.delay_mix=65;eng.delay_time=40;eng.delay_feedback=45;eng.delay_spread=66;}
 if(variant>=3){eng.reverb_type=(uint8_t)(variant-2);eng.reverb_mix=67;eng.reverb_decay=68;eng.reverb_damp=45;}
 const uint32_t count=44100u*3u;
 fputs("RIFF",f);w32(f,36u+count*4u);fputs("WAVEfmt ",f);w32(f,16u);w16(f,1u);w16(f,2u);w32(f,44100u);w32(f,176400u);w16(f,4u);w16(f,16u);fputs("data",f);w32(f,count*4u);
 b1_note_on(&eng,60u,0u,118u);b1_note_on(&eng,64u,0u,118u);b1_note_on(&eng,67u,0u,118u);
 for(uint32_t i=0;i<count;i+=128u){
  if(i>=44100u && i<44100u+128u){b1_note_off(&eng,60u,0u);b1_note_off(&eng,64u,0u);b1_note_off(&eng,67u,0u);}
  b1_render_stereo(&eng,pcm,128u);
  for(uint32_t j=0;j<128u&&i+j<count;j++){w16(f,(uint16_t)pcm[2u*j]);w16(f,(uint16_t)pcm[2u*j+1u]);}
 }
 fclose(f);
}
int main(int argc,char **argv){if(argc!=2)return 2;
 char path[400];const char *names[]={"DRY","CHORUS-C2","DELAY","REVERB-SPRING","REVERB-ROOM","REVERB-HALL","REVERB-CATHEDRAL"};
 for(int i=0;i<7;i++){snprintf(path,sizeof(path),"%s/B1-V2.3-%s.wav",argv[1],names[i]);one(path,i);printf("WAV %s\n",path);}return 0;
}
