/* GPL-3.0-only. Durable auto-live snapshot, old journal migration,
 * power loss fallback, and foreign flash non-erasure. */
#include "b1_storage.h"
#include <assert.h>
#include <stddef.h>
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
 static struct b1_storage state,reopen;
 memset(flash,0xff,BYTES);
 /* Allocate on safe banks while E8000/E9000 belong to another firmware. */
 flash[0x1000u+5u]=0x35u;flash[0x2000u+7u]=0x3cu;
 assert(b1_storage_open(&state,&io)==0&&state.state==B1_STORAGE_EMPTY);
 assert(state.sector[0]!=0xe8000u&&state.sector[1]!=0xe9000u);
 struct b1_project_archive a=state.archive;
 for(unsigned i=0;i<B1_PROJECT_SLOTS;i++){
  project(&a.projects[i],(uint8_t)(10+i));
  b1_storage_default_name((uint8_t)i,a.names[i]);a.mask|=1u<<i;
 }
 /* Simulate EXACT V4.8 schema-2 journal (1696-byte prefix). */
 uint32_t old_len=(uint32_t)offsetof(struct b1_project_archive,live);
 assert(old_len==1696u && sizeof(a)<=4096u-B1_STORAGE_DATA_OFFSET);
 uint32_t at=state.sector[0];uint8_t hdr[32];memset(hdr,0xff,sizeof hdr);
 word(hdr,B1_STORAGE_MAGIC);word(hdr+4,2u);word(hdr+8,1u);
 word(hdr+12,old_len);word(hdr+16,b1_storage_crc(&a,old_len));
 assert(prog(0,at,hdr,32)==0);
 for(uint32_t i=0;i<old_len;){uint32_t n=old_len-i;if(n>256u)n=256u;assert(prog(0,at+B1_STORAGE_DATA_OFFSET+i,((const uint8_t*)&a)+i,n)==0);i+=n;}
 uint8_t commit[4];word(commit,B1_STORAGE_COMMIT);assert(prog(0,at+24,commit,4)==0);
 assert(b1_storage_open(&state,&io)==1);
 assert(state.archive.mask==0xfffu&&!state.archive.live_valid);
 assert(state.archive.projects[4].drive==14u);
 /* Capture live state without modifying any named project. */
 a=state.archive;project(&a.live,72u);a.live_valid=1u;
 assert(b1_storage_commit(&state,&io,&a)==0);
 assert(b1_storage_open(&reopen,&io)==1);
 assert(reopen.archive.live_valid && reopen.archive.live.drive==72u);
 assert(reopen.archive.projects[4].drive==14u&&reopen.archive.mask==0xfffu);
 /* Simulate sudden power loss during inactive-bank programming:
  * previous committed bank must stay readable and intact. */
 struct b1_project_archive bad=reopen.archive;
 project(&bad.live,91u);bad.live_valid=1u;
 fail_at=(int)programs+3;
 assert(b1_storage_commit(&reopen,&io,&bad)!=0);
 fail_at=0;
 assert(b1_storage_open(&reopen,&io)==1);
 assert(reopen.archive.live.drive==72u);
 assert(reopen.archive.projects[4].drive==14u);
 /* Retry committed live snapshot, preserving all project names/data. */
 assert(b1_storage_commit(&reopen,&io,&bad)==0);
 assert(b1_storage_open(&reopen,&io)==1);
 assert(reopen.archive.live.drive==91u && reopen.archive.projects[4].drive==14u);
 assert(flash[0x1000u+5u]==0x35u&&flash[0x2000u+7u]==0x3cu);
 /* A malformed live snapshot may NEVER be committed. */
 uint32_t erase_before=erases;
 bad=reopen.archive;bad.live.checksum^=2u;
 assert(b1_storage_commit(&reopen,&io,&bad)<0&&erases==erase_before);
 printf("PASS: old schema2 journal imported, new autosave restores, 12 named slots unchanged, power-loss rollback, foreign sectors untouched (%u sector erases)\n",erases);
 return 0;
}
