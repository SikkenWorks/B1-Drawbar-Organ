/* SPDX-License-Identifier: GPL-3.0-only. Native schema3 backward compatibility,
 * default OFF, ON/OFF persistence, power-loss and 12-project integrity. */
#include "b1_storage.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BYTES (B1_STORAGE_SCAN_END-B1_STORAGE_SCAN_START)
static uint8_t flash[BYTES];
static uint32_t erases, programs;
static int fail_at;
static int safe(uint32_t at,uint32_t n){return at>=B1_STORAGE_SCAN_START&&n<=BYTES&&at-B1_STORAGE_SCAN_START<=BYTES-n;}
static int rd(void *unused,uint32_t at,uint8_t *p,uint32_t n){(void)unused;if(!safe(at,n))return -1;memcpy(p,flash+at-B1_STORAGE_SCAN_START,n);return 0;}
static int erase(void *unused,uint32_t at){(void)unused;if(!safe(at,4096)||at%4096)return -1;erases++;memset(flash+at-B1_STORAGE_SCAN_START,0xff,4096);return 0;}
static int prog(void *unused,uint32_t at,const uint8_t *p,uint32_t n){
 (void)unused;if(!safe(at,n)||!n||n>256||((at&255)+n)>256)return -1;
 if(fail_at && (int)++programs>=fail_at)return -11;
 uint8_t *q=flash+at-B1_STORAGE_SCAN_START;
 for(uint32_t i=0;i<n;i++){if((q[i]&p[i])!=p[i])return -2;q[i]&=p[i];}
 return 0;
}
static struct b1_storage_io io={0,rd,erase,prog};
static void project(struct b1_project *p,uint8_t drive){
 static struct b1_engine e;static struct b1_performance k;
 b1_reset(&e);b1_performance_init(&k);e.drive=drive;
 b1_project_capture(p,&k,&e);assert(b1_project_valid(p));
}
static void word(uint8_t *b,uint32_t x){for(unsigned i=0;i<4;i++)b[i]=(uint8_t)(x>>(i*8));}
int main(void){
 static struct b1_storage a,b;
 memset(flash,0xff,sizeof(flash));
 /* Keep unknown foreign data in two candidate sectors permanently untouched. */
 flash[0x1000u+5u]=0x35u;flash[0x2000u+7u]=0x3cu;
 assert(b1_storage_open(&a,&io)==0);
 assert(a.autosave_enabled==0);
 struct b1_project_archive arch=a.archive;
 for(unsigned i=0;i<B1_PROJECT_SLOTS;i++){
  project(&arch.projects[i],(uint8_t)(10+i));
  b1_storage_default_name((uint8_t)i,arch.names[i]);arch.mask|=1u<<i;
 }
 project(&arch.live,72u);arch.live_valid=1u;
 /* Simulate *real V4.9/V5.2 schema3 journal*, with 28..31 == FF. */
 uint32_t at=a.sector[0];uint8_t hdr[32];memset(hdr,0xff,sizeof(hdr));
 word(hdr,B1_STORAGE_MAGIC);word(hdr+4,3u);word(hdr+8,1u);
 word(hdr+12,sizeof(arch));word(hdr+16,b1_storage_crc(&arch,sizeof(arch)));
 assert(prog(0,at,hdr,32)==0);
 for(uint32_t i=0;i<sizeof(arch);){uint32_t n=sizeof(arch)-i;if(n>256u)n=256u;
  assert(prog(0,at+B1_STORAGE_DATA_OFFSET+i,((const uint8_t*)&arch)+i,n)==0);i+=n;
 }
 uint8_t commit[4];word(commit,B1_STORAGE_COMMIT);assert(prog(0,at+24,commit,4)==0);
 assert(b1_storage_open(&a,&io)==1);
 assert(a.archive.live_valid==1 && a.archive.live.drive==72u);
 assert(a.autosave_enabled==0); /* Legacy auto-on does NOT migrate as ON. */
 assert(a.archive.mask==0xfffu&&a.archive.projects[4].drive==14u);
 /* Save ON with exact same project archive and same journal schema. */
 a.autosave_enabled=1;
 assert(b1_storage_commit(&a,&io,&a.archive)==0);
 assert(b1_storage_open(&b,&io)==1 && b.autosave_enabled==1);
 assert(b.archive.projects[4].drive==14u && b.archive.live.drive==72u);
 /* After an ON state, simulate power loss during OFF write: must still be ON
  * and retain the exact committed named projects. */
 b.autosave_enabled=0;
 fail_at=(int)programs+3;
 assert(b1_storage_commit(&b,&io,&b.archive)!=0);
 fail_at=0;
 assert(b1_storage_open(&b,&io)==1 && b.autosave_enabled==1);
 assert(b.archive.live.drive==72u && b.archive.projects[4].drive==14u);
 b.autosave_enabled=0;
 assert(b1_storage_commit(&b,&io,&b.archive)==0);
 assert(b1_storage_open(&b,&io)==1 && b.autosave_enabled==0);
 assert(b.archive.mask==0xfffu && b.archive.live.drive==72u);
 /* A corrupted ON marker must fail closed OFF, without erasing project data. */
 uint32_t header_at=b.sector[b.active_bank];
 flash[header_at-B1_STORAGE_SCAN_START+28u]^=0x01u;
 assert(b1_storage_open(&b,&io)==1 && b.autosave_enabled==0);
 assert(b.archive.projects[4].drive==14u);
 assert(flash[0x1000u+5u]==0x35u&&flash[0x2000u+7u]==0x3cu);
 assert(sizeof(struct b1_project_archive)==sizeof(arch));
 printf("PASS: schema3 migration default OFF, ON/OFF persistent, 12 unchanged slots, failed write rollback, foreign sectors safe, %u erases\n",erases);
 return 0;
}
