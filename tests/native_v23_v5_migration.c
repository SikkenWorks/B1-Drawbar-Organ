/* SPDX-License-Identifier: GPL-3.0-only. Demonstrate V2.2 schema-1 -> V2.3 schema-2 migration. */
#include "../firmware/b1_storage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define BYTES (B1_STORAGE_SCAN_END-B1_STORAGE_SCAN_START)
static uint8_t flash[BYTES];
static int rd(void *ctx,uint32_t a,uint8_t *b,uint32_t n){(void)ctx;if(a<B1_STORAGE_SCAN_START||a+n>B1_STORAGE_SCAN_END)return -1;memcpy(b,flash+a-B1_STORAGE_SCAN_START,n);return 0;}
static int erase(void *ctx,uint32_t a){(void)ctx;if(a<B1_STORAGE_SCAN_START||a+4096u>B1_STORAGE_SCAN_END||(a&4095u))return -1;memset(flash+a-B1_STORAGE_SCAN_START,255,4096);return 0;}
static int prog(void *ctx,uint32_t a,const uint8_t *b,uint32_t n){(void)ctx;if(a<B1_STORAGE_SCAN_START||a+n>B1_STORAGE_SCAN_END)return -1;for(uint32_t j=0;j<n;j++){uint8_t *v=&flash[a-B1_STORAGE_SCAN_START+j];if((*v&b[j])!=b[j])return -2;*v&=b[j];}return 0;}
static void put32(uint8_t *dst,uint32_t v){for(int i=0;i<4;i++)dst[i]=(uint8_t)(v>>(i*8));}
static uint32_t hash(const uint8_t *src,uint32_t n){uint32_t h=2166136261u;for(uint32_t i=0;i<n;i++){h^=src[i];h*=16777619u;}return h;}
int main(void){
 memset(flash,255,sizeof flash);
 struct b1_performance p;struct b1_engine e;b1_reset(&e);b1_performance_init(&p);
 e.drawbars[0][1]=5u;p.scale_root=2u;
 struct b1_project newpr;b1_project_capture(&newpr,&p,&e);
 uint8_t archive[1552]={0};put32(archive,1u);
 memcpy(archive+4,"DEEP PURPLE",11);
 memcpy(archive+208,&newpr,108); /* old project data ends at old checksum */
 put32(archive+208+4,5u);put32(archive+208+108,hash(archive+208,108));
 uint8_t hdr[32];memset(hdr,255,sizeof hdr);
 put32(hdr,B1_STORAGE_MAGIC);put32(hdr+4,1u);put32(hdr+8,17u);put32(hdr+12,sizeof archive);
 put32(hdr+16,b1_storage_crc(archive,sizeof archive));put32(hdr+24,B1_STORAGE_COMMIT);
 assert(!prog(NULL,B1_STORAGE_SCAN_START,hdr,sizeof hdr));
 assert(!prog(NULL,B1_STORAGE_SCAN_START+B1_STORAGE_DATA_OFFSET,archive,sizeof archive));
 struct b1_storage_io io={0,rd,erase,prog};struct b1_storage state;
 int result=b1_storage_open(&state,&io);
 assert(result==1 && state.state==B1_STORAGE_READY && state.archive.mask==1u);
 assert(!strcmp(state.archive.names[0],"DEEP PURPLE"));
 assert(b1_project_valid(&state.archive.projects[0]));
 assert(state.archive.projects[0].version==6u && state.archive.projects[0].scale_root==2u);
 assert(state.archive.projects[0].delay_mix==0u && state.archive.projects[0].global_octave==4u);
 /* Next save commits schema-2 archive in a free sector, preserving old schema-1 bank. */
 assert(!b1_storage_commit(&state,&io,&state.archive));
 struct b1_storage loaded;assert(b1_storage_open(&loaded,&io)==1);
 assert(loaded.generation==18u && b1_project_valid(&loaded.archive.projects[0]));
 puts("B1 V2.3: valid V2.2 legacy archive -> V6 project; new schema-2 NOR journal and reboot PASS");
 return 0;
}
