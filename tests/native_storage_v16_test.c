/* GPL-3.0-only. Simulated flash, adversarial power cuts and 12 named projects. */
#include "../firmware/b1_storage.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct fake {uint8_t mem[8192];int budget,ops,erase_count,write_count;};
static int address(uint32_t off,uint32_t n){
 if(off<B1_STORAGE_A || n>8192u || off-B1_STORAGE_A>8192u-n)return -1;
 return (int)(off-B1_STORAGE_A);
}
static int gate(struct fake *f){f->ops++;if(f->budget>=0 && --f->budget<0)return -1;return 0;}
static int rd(void *v,uint32_t addr,uint8_t *out,uint32_t n){
 struct fake *f=v;int a=address(addr,n);if(a<0)return -1;memcpy(out,f->mem+a,n);return 0;
}
static int er(void *v,uint32_t addr){
 struct fake *f=v;int a=address(addr,4096u);
 if(a<0|| (addr&4095u) || gate(f))return -1;
 memset(f->mem+a,0xff,4096u);f->erase_count++;return 0;
}
static int wr(void *v,uint32_t addr,const uint8_t *src,uint32_t n){
 struct fake *f=v;int a=address(addr,n);
 if(a<0||!n||n>256u||((addr&255u)+n)>256u||gate(f))return -1;
 for(unsigned i=0;i<n;i++)if((uint8_t)(f->mem[a+i]&src[i])!=src[i])return -2;
 for(unsigned i=0;i<n;i++)f->mem[a+i]&=src[i];
 f->write_count++;return 0;
}
static struct b1_storage_io io(struct fake *f){return (struct b1_storage_io){f,rd,er,wr};}
static void make_project(struct b1_project *p,unsigned seed){
 struct b1_engine e;struct b1_performance perf;
 b1_reset(&e);b1_performance_init(&perf);
 for(unsigned i=0;i<9;i++)e.drawbars[0][i]=(uint8_t)((i+seed)%9);
 perf.range[2].attack=(uint8_t)(seed%101);
 b1_project_capture(p,&perf,&e);
 assert(b1_project_valid(p));
}
int main(void){
 struct fake *f=malloc(sizeof(*f));assert(f);memset(f,0xff,sizeof(*f));f->budget=-1;
 struct b1_storage s;struct b1_storage_io ops=io(f);
 assert(b1_storage_open(&s,&ops)==0 && s.state==B1_STORAGE_EMPTY);
 struct b1_project_archive work=s.archive;
 assert(sizeof(work)<3840u);
 for(unsigned k=0;k<12;k++){
  b1_storage_default_name((uint8_t)k,work.names[k]);
  make_project(&work.projects[k],k);
  work.mask|=1u<<k;
 }
 /* First-ever save may be cut at any flash operation: a subsequent boot
  * still permits a new save if the sector is blank or B1-owned/uncommitted. */
 for(int budget=0;budget<16;budget++){
  memset(f->mem,0xff,sizeof(f->mem));f->budget=budget;f->ops=0;
  struct b1_storage first;assert(b1_storage_open(&first,&ops)==0);
  (void)b1_storage_commit(&first,&ops,&work);
  f->budget=-1;
  struct b1_storage after;int status=b1_storage_open(&after,&ops);
  assert(status==0 || status==1);
  if(status==0)assert(b1_storage_commit(&after,&ops,&work)==0);
  assert(b1_storage_open(&after,&ops)==1);
 }
 memset(f->mem,0xff,sizeof(f->mem));f->budget=-1;
 assert(b1_storage_open(&s,&ops)==0);
 assert(b1_storage_commit(&s,&ops,&work)==0);
 assert(s.archive.mask==4095u && s.active_bank==0);
 struct b1_storage reopened;
 assert(b1_storage_open(&reopened,&ops)==1);
 assert(reopened.archive.mask==4095u);
 for(unsigned k=0;k<12;k++){
  assert(strcmp(reopened.archive.names[k],work.names[k])==0);
  assert(b1_project_valid(&reopened.archive.projects[k]));
 }
 /* Saving one slot never destroys or renames the other eleven. */
 strcpy(work.names[11],"DEEP PURPLE");make_project(&work.projects[11],99);
 assert(b1_storage_commit(&s,&ops,&work)==0);
 assert(b1_storage_open(&reopened,&ops)==1);
 assert(strcmp(reopened.archive.names[11],"DEEP PURPLE")==0);
 assert(strcmp(reopened.archive.names[0],"PROJECT 01")==0);
 /* Simulate a power cut after every possible SPI program/erase operation.
  * The previously committed generation must remain readable. */
 struct fake baseline=*f;
 for(int budget=0;budget<24;budget++){
  *f=baseline;f->budget=budget;f->ops=0;
  struct b1_storage t;
  assert(b1_storage_open(&t,&ops)==1);
  struct b1_project_archive updated=t.archive;
  strcpy(updated.names[0],"THE STRANGLER");make_project(&updated.projects[0],44);
  int rc=b1_storage_commit(&t,&ops,&updated);
  f->budget=-1;
  assert(b1_storage_open(&reopened,&ops)==1);
  assert(reopened.archive.mask==4095u);
  assert(!strcmp(reopened.archive.names[11],"DEEP PURPLE"));
  if(rc==0)assert(!strcmp(reopened.archive.names[0],"THE STRANGLER"));
  else assert(!strcmp(reopened.archive.names[0],"PROJECT 01"));
 }
 *f=baseline;f->budget=-1;
 /* Repeated writes alternate safely. */
 for(unsigned i=0;i<40;i++){
  struct b1_project_archive a=s.archive;
  a.projects[0].drawbars[0][0]=9u;
  /* A project checksum must be valid or the writer rejects the archive. */
  assert(b1_storage_commit(&s,&ops,&a)<0);
  a=s.archive;make_project(&a.projects[0],i);
  assert(b1_storage_commit(&s,&ops,&a)==0);
 }
 assert(b1_storage_open(&reopened,&ops)==1);
 /* Corrupt the newest active bank. Boot picks the older intact snapshot. */
 f->mem[(reopened.active_bank?4096u:0u)+256u]^=0x01u;
 assert(b1_storage_open(&reopened,&ops)==1);
 assert(reopened.archive.mask==4095u);
 /* Foreign bytes in the unused bank must block writes into that bank. */
 memset(f->mem,0xff,8192u);f->mem[4096u+7u]=0x39u;
 assert(b1_storage_open(&reopened,&ops)==-3);
 int was=f->erase_count;
 assert(b1_storage_commit(&reopened,&ops,&work)<0 && f->erase_count==was);
 assert(f->mem[4096u+7u]==0x39u);
 printf("PASS: 12 named slots, journal CRC/commit, first-save cuts 0..15, update cuts 0..23, foreign preservation, 40 consecutive commits, reboot/fallback. archive=%zu bytes\n",sizeof(work));
 free(f);return 0;
}
