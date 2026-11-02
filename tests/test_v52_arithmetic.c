/* Verify V5.2's exact 32-bit ROCK fast path against the V5.1 64-bit expressions.
 * Includes source to exercise static private DSP helpers without altering firmware ABI. */
#include <stdint.h>
#include <stdio.h>
#include "../firmware/b1_engine.c"
static uint32_t rng=0x726F636Bu;
static uint32_t next(void){rng=rng*1664525u+1013904223u;return rng;}
int main(void){
 for(unsigned i=0;i<500000;i++){
  int32_t a=(int32_t)(next()%12000001u)-6000000;
  int32_t b=(int32_t)(next()%12000001u)-6000000;
  uint32_t mix=next()%101u;
  int32_t fast=b1_rock_cabinet_exact(a,b,mix);
  int32_t original=(int32_t)(((int64_t)a*(100u-mix)+(int64_t)b*mix)/100);
  if(fast!=original){printf("CAB FAIL %u\n",i);return 1;}
  uint32_t blend=next()%32768u;
  fast=b1_rock_crossfade_exact(a,b,blend);
  original=(int32_t)(((int64_t)a*(32767u-blend)+(int64_t)b*blend)/32767);
  if(fast!=original){printf("XFADE FAIL %u\n",i);return 1;}
  int32_t coefficient=(int32_t)(next()%1557u);
  fast=b1_mul_div256_fast(a,coefficient);
  original=(int32_t)(((int64_t)a*coefficient)/256);
  if(fast!=original){printf("Q8 FAIL %u\n",i);return 1;}
 }
 puts("PASS: 1,500,000 exact full-width checks (including fallback) against V5.1 64-bit ROCK arithmetic");
 return 0;
}
