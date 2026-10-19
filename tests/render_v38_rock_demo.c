/* SPDX-License-Identifier: GPL-3.0-only
 * Offline audition. The resulting WAV is not a recording of actual FM1 audio.
 * Both examples use the same original chord progression and 91-wheel engine.
 * CPU relief is the actual SAFE DSP startup branch used by B1 on FM1.
 */
#include "../firmware/b1_engine.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static struct b1_engine e;
static int16_t pcm[256];
static void wr16(FILE *f,unsigned n){ fputc(n&255u,f);fputc((n>>8)&255u,f);}
static void wr32(FILE *f,uint32_t n){wr16(f,n&65535u);wr16(f,n>>16);}
int main(int argc,char **argv){
 if(argc!=3){fprintf(stderr,"usage: render_v38_rock_demo ROCK|B3 file.wav\n");return 2;}
 int rock=argv[1][0]=='R';if(!rock && argv[1][0]!='B')return 2;
 FILE *f=fopen(argv[2],"wb");if(!f){perror("WAV");return 2;}
 enum { BLOCKS=1600, FRAMES=128, RATE=44100 };
 const uint32_t bytes=BLOCKS*FRAMES*2u*sizeof(int16_t);
 fwrite("RIFF",1,4,f);wr32(f,bytes+36u);fwrite("WAVEfmt ",1,8,f);
 wr32(f,16u);wr16(f,1u);wr16(f,2u);wr32(f,RATE);wr32(f,RATE*4u);
 wr16(f,4u);wr16(f,16u);fwrite("data",1,4,f);wr32(f,bytes);
 b1_reset(&e);e.cpu_relief=1u;e.osc_mode=B1_OSC_ORGAN;
 e.sound_mode=(uint8_t)(rock?B1_SOUND_ROCK:B1_SOUND_B3);
 e.mode_mix_q15=(uint16_t)(rock?32767u:0u);
 e.drive=(uint8_t)(rock?72u:18u);e.amp_gain=82u;e.amp_tone=65u;e.cabinet_mix=82u;
 e.click=23u;e.leakage=7u;e.percussion_enabled=1u;e.percussion_harmonic=3u;
 e.rotor_enabled=(uint8_t)!rock;e.rotor_mix=rock?0u:75u;
 e.cv_mode=0u;e.reverb_type=0u;e.delay_mix=0u;
 const uint8_t pull[9]={8,8,8,8,0,0,0,0,0};
 for(unsigned h=0;h<9;h++)b1_drawbar(&e,0,(uint8_t)h,pull[h]);
 const uint8_t notes[8][3]={
  {48,55,60},{51,58,63},{53,60,65},{55,62,67},
  {48,60,67},{46,53,58},{51,58,65},{48,55,60}
 };
 uint32_t peak=0u;
 for(unsigned block=0;block<BLOCKS;block++){
  unsigned chord=block/200u;
  if(block%200u==0u){
   for(unsigned k=0;k<3;k++)b1_note_on_tagged(&e,notes[chord][k],0u,110u,chord*3u+k+1u);
  }else if(block%200u==153u){
   for(unsigned k=0;k<3;k++)b1_note_off_tagged(&e,chord*3u+k+1u);
  }
  b1_render_stereo(&e,pcm,FRAMES);
  for(unsigned t=0;t<256;t++){
   int32_t n=pcm[t];uint32_t a=(uint32_t)(n<0?-n:n);
   if(a>peak)peak=a;
   wr16(f,(uint16_t)pcm[t]);
  }
 }
 if(fclose(f)){perror("closing wave");return 1;}
 printf("Generated original %s tonewheel progression, peak %u, %u frames\n",argv[1],peak,BLOCKS*FRAMES);
 return peak<30000u?0:1;
}
