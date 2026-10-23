#include "b1_engine.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
static struct b1_engine a,b;
static int16_t pcm_a[512],pcm_b[512];
int main(void){
 int bad=0;
 for(int voice=1;voice<=12;voice++)for(int opt=0;opt<4;opt++){
  b1_reset(&a);a.cpu_relief=1u; a.click=opt&1?67:0;
  a.osc_mode=(opt>>1)&1u;
  a.leakage=27;a.percussion_enabled=1;a.rotor_enabled=1;a.rotor_mix=67;
  for(int bank=0;bank<3;bank++)for(int h=0;h<9;h++)b1_drawbar(&a,bank,h,8);
  for(int v=0;v<voice;v++)b1_note_on(&a,47+3*v,v%3,100);
  memcpy(&b,&a,sizeof a);
  b1_render_stereo(&a,pcm_a,256);
  b1_render_stereo(&b,pcm_b,128);
  b1_render_stereo(&b,pcm_b+256,128);
  int same=!memcmp(pcm_a,pcm_b,sizeof pcm_a);
  if(!same){bad++;printf("DIFF voice=%d opt=%d firstL %d/%d\n",voice,opt,pcm_a[256],pcm_b[256]);}
 }
 printf("CHUNK CONSISTENCY: %d/48 differences\n",bad);
 return bad?1:0;
}
