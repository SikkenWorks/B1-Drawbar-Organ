/* B1 V3.1: exact signed divisor and sag-path equivalence test. */
#include <stdio.h>
#include <stdint.h>
#include "../firmware/b1_engine.c"
static uint32_t seed=0x1fac379bu;
static uint32_t random32(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
int main(void){
 for(uint32_t i=0;i<250000u;i++){
   int32_t x=(int32_t)(random32()%65536u)-32768;
   int32_t d=(int32_t)(random32()%8u)+2;
   if(b1_reverb_div(x,d,65536u/(uint32_t)d)!=x/d){printf("reverb mismatch at %d, %d\n",x,d);return 1;}
   int32_t magnitude=(int32_t)(random32()%16001u);
   int32_t sag=(int32_t)(random32()%16001u);
   int32_t before=sag+(magnitude-sag)/(magnitude>sag?128:4096);
   int32_t delta=magnitude-sag;
   int32_t after=sag+(delta>=0?delta/128:delta/4096);
   if(before!=after){puts("sag mismatch");return 2;}
 }
 puts("250000 signed reverb/sag comparisons: EXACT");
 return 0;
}
