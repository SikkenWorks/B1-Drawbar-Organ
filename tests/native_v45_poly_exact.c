/* GPL-3.0-only. Deterministic sound + complete DSP state verification vs V4.4.
 * Compile once with tests/reference_v41/b1_engine.c (byte-identical V4.4),
 * once with current firmware/b1_engine.c, and compare stdout exactly. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "b1_engine.h"
static struct b1_engine e;
static int16_t pcm[256];
static uint64_t hash=UINT64_C(14695981039346656037);
static void bytes(const void *ptr,size_t n){const uint8_t *s=ptr; for(size_t i=0;i<n;i++){hash^=s[i];hash*=UINT64_C(1099511628211);}}
static uint32_t rng=0x51ce1253u;
static uint32_t rand32(void){rng=rng*1664525u+1013904223u;return rng;}
static void block(unsigned n){b1_render_stereo(&e,pcm,n);bytes(pcm,(size_t)n*4u);}
static void run(unsigned n){while(n>=128u){block(128u);n-=128u;}if(n)block(n);}
int main(void){
 for(unsigned voices=1u;voices<=12u;voices++)for(unsigned cfg=0u;cfg<8u;cfg++){
  hash=UINT64_C(14695981039346656037);rng=0x51ce1253u;
  b1_reset(&e); e.cpu_relief=(uint8_t)(cfg&1u);e.sound_mode=(uint8_t)((cfg>>1)&1u);
  e.osc_mode=(uint8_t)((cfg>>2)&1u);e.click=(uint8_t)((cfg&1u)?67u:0u);
  e.leakage=cfg&1u?37u:0u;e.percussion_enabled=1u;
  e.rotor_enabled=1u;e.rotor_mix=(uint8_t)(cfg&1u?75u:40u);
  e.cv_mode=(uint8_t)(cfg&1u?4u:2u);e.cv_depth=54u;
  e.delay_mix=(uint8_t)(cfg&1u?44u:0u);e.reverb_type=(uint8_t)(cfg&1u?3u:0u);e.reverb_mix=(uint8_t)(cfg&1u?34u:0u);
  for(unsigned b=0;b<3;b++)for(unsigned h=0;h<9;h++)b1_drawbar(&e,(uint8_t)b,(uint8_t)h,(uint8_t)(rand32()%9u));
  if(cfg==7u){
   e.click=255u;e.leakage=255u;e.percussion_harmonic=2u;
   for(unsigned bank=0;bank<3;bank++)for(unsigned h=0;h<9;h++)b1_drawbar(&e,(uint8_t)bank,(uint8_t)h,8u);
  }
  for(unsigned j=0;j<voices;j++)b1_note_on(&e,(uint8_t)(44u+j*3u),(uint8_t)(j%3u),100u);
  for(unsigned k=0;k<36;k++)block(128u);
  bytes(&e,sizeof(e));
  for(unsigned j=0;j<voices;j+=2u)b1_note_off(&e,(uint8_t)(44u+j*3u),(uint8_t)(j%3u));
  for(unsigned k=0;k<30;k++)block(128u);
  bytes(&e,sizeof(e));
  b1_panic(&e);run(16111u);bytes(&e,sizeof(e));
  printf("voice=%u cfg=%u %016llx\n",voices,cfg,(unsigned long long)hash);
 }
}
