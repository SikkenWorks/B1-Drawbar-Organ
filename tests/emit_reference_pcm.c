/* Emit a reproducible 44.1 kHz stereo PCM16 A/B reference fixture.
 * Run with v1.0 sources for golden baseline or current sources to compare. */
#include "b1_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
static struct b1_engine audio;
static int16_t pcm[128*2];
int main(int argc,char **argv){
 if(argc!=4){fprintf(stderr,"usage: emit_reference_pcm MODE VOICES OUTPUT\n");return 2;}
 int mode=atoi(argv[1]), voices=atoi(argv[2]);if(mode<0||mode>1||voices<0||voices>12)return 2;
 FILE *out=fopen(argv[3],"wb");if(!out)return 3;
 b1_reset(&audio);audio.sound_mode=(uint8_t)mode;audio.mode_mix_q15=(uint16_t)(mode?32767:0);
 audio.cv_mode=5;audio.rotor_fast=1;
 for(int b=0;b<3;b++)for(int h=0;h<9;h++)b1_drawbar(&audio,(uint8_t)b,(uint8_t)h,(uint8_t)(1+((b+h)%8)));
 for(int v=0;v<voices;v++)b1_note_on_tagged(&audio,(uint8_t)(49+v),(uint8_t)(v%3),112,(uint32_t)v+1);
 for(int block=0;block<32;block++){
  b1_render_stereo(&audio,pcm,128);
  if(fwrite(pcm,sizeof(pcm),1,out)!=1){fclose(out);return 4;}
 }
 fclose(out);return 0;
}
