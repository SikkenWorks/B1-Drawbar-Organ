/* GPL-3.0-only. Standalone 32-bit fixed-point exactness proof by testing. */
#include "../firmware/b1_engine.c"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
int main(void){
 uint32_t s=0xABCD4321u;
 const uint16_t fixed[]={0,1,127,850,7800,12500,17000,23975,32767};
 const int32_t edges[]={INT32_MIN,INT32_MAX,-32768,-1,0,1,32767,65535,1234567,-1234567};
 unsigned long long checks=0;
 for(unsigned i=0;i<sizeof(fixed)/sizeof(fixed[0]);i++){
  for(unsigned k=0;k<sizeof(edges)/sizeof(edges[0]);k++){
   int32_t x=edges[k];uint16_t g=fixed[i];
   int32_t want=(int32_t)(((int64_t)x*(int64_t)g)/32768);
   assert(b1_filter_q15_exact(x,g)==want);checks++;
  }
 }
 for(unsigned i=0;i<2500000u;i++){
  s^=s<<13;s^=s>>17;s^=s<<5;
  int32_t x=(int32_t)s;
  uint16_t g=(uint16_t)((s>>10)&32767u);
  int32_t want=(int32_t)(((int64_t)x*(int64_t)g)/32768);
  assert(b1_filter_q15_exact(x,g)==want);checks++;
 }
 const uint16_t *tables[]={b1_clip_16500,b1_clip_20500,b1_clip_30000};
 unsigned long long clip_checks=0;
 for(unsigned t=0;t<3u;t++)for(unsigned at=0;at<2048u;at++){
  uint32_t a=tables[t][at],b=tables[t][at+1u];
  assert(b>=a); /* exactness relies on generated tables being monotone */
  for(uint32_t f=0;f<512u;f++){
   uint32_t old_y=(a*(512u-f)+b*f)>>9;
   uint32_t new_y=a+(((b-a)*f)>>9);
   assert(new_y==old_y);
   clip_checks++;
  }
 }
 printf("PASS %llu exact Q15 signed-division cases (including INT32_MIN); %llu exhaustive softclip LUT interpolations\n",checks,clip_checks);
 return 0;
}
