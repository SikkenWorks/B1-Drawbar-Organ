/* B1 V1.7: full 0xE7000..EFFFF virtual NOR, with foreign E8000/E9000. */
#include "../firmware/b1_storage.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BYTES (B1_STORAGE_SCAN_END-B1_STORAGE_SCAN_START)
static uint8_t mem[BYTES];
static uint32_t writes[BYTES/B1_STORAGE_SECTOR];
static int inrange(uint32_t at,uint32_t n){return at>=B1_STORAGE_SCAN_START && n<=BYTES && at-B1_STORAGE_SCAN_START<=BYTES-n;}
static int rd(void *x,uint32_t at,uint8_t *dst,uint32_t n){(void)x;if(!inrange(at,n))return -1;memcpy(dst,mem+at-B1_STORAGE_SCAN_START,n);return 0;}
static int erase(void *x,uint32_t at){(void)x;if(!inrange(at,4096)||at%4096)return -1;memset(mem+at-B1_STORAGE_SCAN_START,0xff,4096);writes[(at-B1_STORAGE_SCAN_START)/4096]++;return 0;}
static int prog(void *x,uint32_t at,const uint8_t *src,uint32_t n){(void)x;if(!inrange(at,n)||!n||n>256||((at&255)+n)>256)return -1;for(unsigned i=0;i<n;i++)if((mem[at-B1_STORAGE_SCAN_START+i]&src[i])!=src[i])return -2;for(unsigned i=0;i<n;i++)mem[at-B1_STORAGE_SCAN_START+i]&=src[i];writes[(at-B1_STORAGE_SCAN_START)/4096]++;return 0;}
static void project(struct b1_project *out){struct b1_engine e;struct b1_performance p;b1_reset(&e);b1_performance_init(&p);b1_project_capture(out,&p,&e);}
int main(void){
 struct b1_storage_io io={0,rd,erase,prog};struct b1_storage state,reopen;
 memset(mem,0xff,sizeof mem);
 /* Simulate actual user's unexpected non-B1 bytes in the original sectors. */
 mem[0x1000+5]=0xa1;mem[0x2000+7]=0x5b;
 uint8_t before1=mem[0x1000+5],before2=mem[0x2000+7];
 assert(b1_storage_open(&state,&io)==0 && state.state==B1_STORAGE_EMPTY);
 assert(state.sector[0]!=0xE8000 && state.sector[0]!=0xE9000);
 assert(state.sector[1]!=0xE8000 && state.sector[1]!=0xE9000);
 struct b1_project_archive a=state.archive;
 for(unsigned i=0;i<12;i++){project(&a.projects[i]);b1_storage_default_name(i,a.names[i]);a.mask|=1u<<i;}
 assert(b1_storage_commit(&state,&io,&a)==0);
 assert(b1_storage_open(&reopen,&io)==1 && reopen.archive.mask==0xfff);
 assert(reopen.archive.names[11][0]=='P');
 assert(mem[0x1000+5]==before1 && mem[0x2000+7]==before2);
 assert(writes[1]==0 && writes[2]==0);
 /* All nine sectors contain foreign data: fail closed, never erase. */
 memset(mem,0x53,sizeof mem);memset(writes,0,sizeof writes);
 assert(b1_storage_open(&state,&io)==-3 && state.state==B1_STORAGE_LOCKED);
 assert(b1_storage_commit(&state,&io,&a)<0);
 for(unsigned i=0;i<BYTES/B1_STORAGE_SECTOR;i++)assert(writes[i]==0);
 puts("PASS: dynamic safe-sector discovery, persistent 12 slots, foreign sectors unchanged, full-region lock");
 return 0;
}
