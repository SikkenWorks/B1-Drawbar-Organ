/* SPDX-License-Identifier: GPL-3.0-only
 * B1 V3.0: verify new exact unsigned magnitude and ramp short path.
 * The optimized ramp is included by the compilation unit to test the
 * actual static function; run alongside a literal V2.9 reference.
 */
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include "../firmware/b1_engine.c"
static uint32_t old_ramp(uint32_t v,uint32_t target,uint32_t tau){
 if(v==target)return v;
 if(v<target){uint32_t d=(target-v+tau-1u)/tau;return d>target-v?target:v+d;}
 uint32_t d=(v-target+tau-1u)/tau;return d>v-target?target:v-d;
}
static uint32_t rnd=0x7ca2b112u;
static uint32_t next(void){rnd^=rnd<<13;rnd^=rnd>>17;rnd^=rnd<<5;return rnd;}
int main(void){
 int32_t special[]={INT32_MIN,-2000000,-32768,-1,0,1,32767,2000000,INT32_MAX};
 for(uint32_t i=0;i<sizeof(special)/sizeof(special[0]);i++){
  int32_t n=special[i];uint32_t expected=n<0?0u-(uint32_t)n:(uint32_t)n;
  if(b1_abs32(n)!=expected){fprintf(stderr,"bad abs at %u\n",i);return 1;}
 }
 for(uint32_t i=0;i<250000u;i++){
  uint32_t v=next()%2000000u,target=next()%2000000u,tau=(next()%240000u)+1u;
  uint32_t old=old_ramp(v,target,tau),got=ramp(v,target,tau);
  if(old!=got){fprintf(stderr,"ramp difference %u %u %u: %u != %u\n",v,target,tau,old,got);return 1;}
 }
 puts("B1 V3.0: 250000 exact ramp comparisons and signed abs boundaries PASS");
 return 0;
}
