/* V5.0 differential verification versus pristine V4.9.
 * Test input is deterministic; output includes PCM and all DSP fields.
 * Compiled once with tests/reference_v49/b1_engine.c, once with current engine.
 */
#include <stdint.h>
#include <stdio.h>
#include "b1_engine.h"
static struct b1_engine e;
static int16_t pcm[512];
static uint32_t rng;
static uint64_t h;
static uint32_t rand32(void){rng=rng*1664525u+1013904223u;return rng;}
static void add(const void *p,unsigned n){const uint8_t *x=p;for(unsigned k=0;k<n;k++){h^=x[k];h*=UINT64_C(1099511628211);}}
int main(void){
 for(uint32_t cfg=0;cfg<128u;cfg++){
  b1_reset(&e);rng=0xada61720u+cfg*31u;h=UINT64_C(14695981039346656037);
  e.cpu_relief=(uint8_t)(cfg&1u);
  e.sound_mode=(uint8_t)((cfg>>1)&1u);
  e.osc_mode=(uint8_t)((cfg>>2)&1u);
  e.click=(uint8_t)((cfg*37u)%256u);
  e.leakage=(uint8_t)((cfg*17u)%101u);
  e.percussion_enabled=1u;e.rotor_mix=(uint8_t)((cfg*11u)%101u);
  e.cv_mode=(uint8_t)(cfg%7u);e.cv_depth=(uint8_t)(cfg%101u);
  e.delay_mix=(uint8_t)((cfg*13u)%101u);
  e.reverb_type=(uint8_t)(cfg%4u);e.reverb_mix=(uint8_t)((cfg*7u)%101u);
  for(uint32_t b=0;b<3;b++)for(uint32_t bar=0;bar<9;bar++)b1_drawbar(&e,(uint8_t)b,(uint8_t)bar,(uint8_t)(rand32()%9u));
  uint8_t nvoices=(uint8_t)(1u+(cfg%12u));
  for(uint32_t j=0;j<nvoices;j++)b1_note_on_tagged(&e,(uint8_t)(37u+j*4u),(uint8_t)(j%3),100,j+1u);
  for(uint32_t block=0;block<28;block++){
   if(block==4u)for(uint32_t k=0;k<3u;k++)b1_drawbar(&e,(uint8_t)k,(uint8_t)(cfg%9u),(uint8_t)(rand32()%9u));
   if(block==8u)for(uint32_t j=0;j<nvoices;j+=2u)b1_note_off_tagged(&e,j+1u);
   if(block==12u)for(uint32_t j=0;j<nvoices;j+=2u)b1_note_on_tagged(&e,(uint8_t)(41u+j*4u),(uint8_t)(j%3),115,j+30u);
   if(block==17u)e.osc_mode=(uint8_t)(!e.osc_mode);
   if(block==20u)e.sound_mode=(uint8_t)(!e.sound_mode);
   b1_render_stereo(&e,pcm,256u);
   add(pcm,sizeof(pcm));
   if(block==8u || block==13u || block==27u)add(&e,sizeof(e));
  }
  printf("case %u %016llx\n",cfg,(unsigned long long)h);
 }
 return 0;
}
