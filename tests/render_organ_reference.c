/* Optional offline audio reference rendered with the EXACT b1_engine.c DSP.
 * Not a simulator, hardware recording, or reference to any copyrighted song.
 * This is to allow subjective evaluation of our tonewheel sound.
 */
#include "../firmware/b1_engine.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static struct b1_engine e;
static int16_t pcm[64*2];
static void w16(FILE *f,uint16_t v){fputc((int)(v&255),f);fputc((int)(v>>8),f);}
static void w32(FILE *f,uint32_t v){w16(f,(uint16_t)v);w16(f,(uint16_t)(v>>16));}
int main(int argc,char **argv){
 if(argc!=3){fprintf(stderr,"usage: b1-render-audio OUT.wav clean|rock\n");return 2;}
 int rock=(argv[2][0]=='r');
 const uint32_t seconds=6,frames=B1_SAMPLE_RATE*seconds;
 FILE *out=fopen(argv[1],"wb");if(!out){perror(argv[1]);return 3;}
 fwrite("RIFF",1,4,out);w32(out,36u+frames*4u);fwrite("WAVEfmt ",1,8,out);
 w32(out,16);w16(out,1);w16(out,2);w32(out,B1_SAMPLE_RATE);w32(out,B1_SAMPLE_RATE*4);
 w16(out,4);w16(out,16);fwrite("data",1,4,out);w32(out,frames*4u);
 b1_reset(&e);e.cpu_relief=1u;e.sound_mode=rock?B1_SOUND_ROCK:B1_SOUND_B3;e.mode_mix_q15=rock?32767u:0u;
 e.drive=rock?80u:0u;e.amp_gain=rock?78u:35u;e.amp_tone=rock?67u:50u;e.cabinet_mix=rock?95u:0u;
 e.rotor_mix=rock?0u:85u;
 e.cv_mode=rock?0:5;
 uint8_t chord[4]={55,58,62,67};
 for(uint32_t i=0;i<frames;){
  if(i==0u){for(int j=0;j<3;j++)b1_note_on(&e,chord[j],0,110);}
  if(i>=44100u&&i<44100u+64u){b1_note_on(&e,chord[3],0,108);e.rotor_fast=1u;}
  if(i>=88200u&&i<88200u+64u){b1_note_off(&e,chord[0],0);b1_note_off(&e,chord[1],0);b1_note_off(&e,chord[2],0);}
  if(i>=132300u&&i<132300u+64u){b1_note_off(&e,chord[3],0);b1_note_on(&e,60,0,100);b1_note_on(&e,64,0,100);b1_note_on(&e,67,0,100);}
  if(i>=220500u&&i<220500u+64u){b1_note_off(&e,60,0);b1_note_off(&e,64,0);b1_note_off(&e,67,0);}
  uint32_t n=64u;if(frames-i<n)n=frames-i;
  b1_render_stereo(&e,pcm,n);for(uint32_t s=0;s<n*2u;s++)w16(out,(uint16_t)pcm[s]);
  i+=n;
 }
 fclose(out);
 printf("WAV %s, %u samples, %s: rendered from B1 V2.2 SAFE DSP\n",argv[1],frames,rock?"rock":"clean");
 return 0;
}
