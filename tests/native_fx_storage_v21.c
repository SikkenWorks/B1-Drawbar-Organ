/* Exact firmware DSP linked on host, no simulated webaudio. */
#include "../firmware/b1_engine.h"
#include "../firmware/b1_storage.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
static struct b1_engine a,b;
static int16_t left[512*2],right[512*2];
static void prepare(struct b1_engine *e){
 b1_reset(e);e->cpu_relief=1;e->click=0;e->leakage=0;
 e->drive=35;e->rotor_width=85;e->rotor_mix=95;
 b1_note_on(e,53,0,100);b1_note_on(e,60,0,100);b1_note_on(e,64,0,100);
}
static uint64_t contrast(uint8_t cm0,uint8_t cm1,uint8_t rotor0,uint8_t rotor1,uint8_t fast0,uint8_t fast1){
 prepare(&a);prepare(&b);
 a.cv_mode=cm0;b.cv_mode=cm1;
 a.rotor_mix=rotor0;b.rotor_mix=rotor1;
 a.rotor_fast=fast0;b.rotor_fast=fast1;
 uint64_t total=0, energy=0;
 for(int j=0;j<50;j++){
  b1_render_stereo(&a,left,512);b1_render_stereo(&b,right,512);
  if(j<4)continue;
  for(int i=0;i<1024;i++){
   int32_t d=(int32_t)left[i]-(int32_t)right[i];
   total+=(uint64_t)(d<0?-d:d);
   energy+=(uint64_t)(right[i]<0?-right[i]:right[i]);
  }
 }
 printf("audio difference avg=%" PRIu64 " signal avg=%" PRIu64 "\n",total/(46u*1024u),energy/(46u*1024u));
 return total;
}
#define SIZE (B1_STORAGE_SCAN_END-B1_STORAGE_SCAN_START)
static uint8_t flash[SIZE];static int erase_cnt,read_cnt;
static int rd(void *x,uint32_t a,uint8_t *o,uint32_t n){(void)x;read_cnt++;if(a<B1_STORAGE_SCAN_START ||a+n>B1_STORAGE_SCAN_END)return -1;memcpy(o,flash+a-B1_STORAGE_SCAN_START,n);return 0;}
static int er(void *x,uint32_t a){(void)x;if(a<B1_STORAGE_SCAN_START||a+4096u>B1_STORAGE_SCAN_END||(a&4095u))return -1;erase_cnt++;memset(flash+a-B1_STORAGE_SCAN_START,255,4096);return 0;}
static int pg(void *x,uint32_t a,const uint8_t *v,uint32_t n){(void)x;if(a<B1_STORAGE_SCAN_START||a+n>B1_STORAGE_SCAN_END)return -1;for(uint32_t i=0;i<n;i++)flash[a-B1_STORAGE_SCAN_START+i]&=v[i];return 0;}
int main(void){
 /* Effects must audibly alter the VERY BRANCH used by the FM1 hardware. */
 assert(contrast(0,5,0,0,0,0)>300000u); /* C2 at full intensity */
 assert(contrast(0,2,0,0,0,0)>300000u); /* V2 */
 assert(contrast(0,0,0,100,0,0)>300000u); /* full Leslie */
 assert(contrast(0,0,100,100,0,1)>200000u); /* ramped SLOW/FAST */
 struct b1_storage s;struct b1_storage_io io={0,rd,er,pg};
 memset(flash,0x59,sizeof flash);
 assert(b1_storage_open(&s,&io)==-3);assert(s.state==B1_STORAGE_LOCKED);
 assert(b1_storage_reclaim_confirmed(&s,&io,0u)<0);assert(erase_cnt==0);
 assert(b1_storage_reclaim_confirmed(&s,&io,1u)==0);
 assert(erase_cnt==2);assert(s.state==B1_STORAGE_EMPTY);
 /* Only two bounded sectors changed; every other byte was untouched. */
 uint32_t count=0;
 for(uint32_t i=0;i<SIZE;i++)if(flash[i]!=0x59)count++;
 assert(count==8192u);
 /* On a clean reinstall no destructive prompt or additional erase. */
 assert(b1_storage_open(&s,&io)==0);
 assert(b1_storage_reclaim_confirmed(&s,&io,1u)<0);assert(erase_cnt==2);
 printf("V2.1 PASS effects + two-step erase guard; modified bytes=%u\n",count);
 return 0;
}
