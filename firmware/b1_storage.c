/* SPDX-License-Identifier: GPL-3.0-only
 * Journal guarantees: only the *inactive* 4 KiB sector is erased; the current
 * committed sector remains intact until the replacement passes full readback.
 * The four-byte COMMIT is programmed LAST. Unknown flash is never erased.
 */
#include "b1_storage.h"
#define HDR_MAGIC 0u
#define HDR_SCHEMA 4u
#define HDR_GEN 8u
#define HDR_LEN 12u
#define HDR_CRC 16u
#define HDR_COMMIT 24u
_Static_assert(sizeof(struct b1_project_archive)<=B1_STORAGE_SECTOR-B1_STORAGE_DATA_OFFSET,
 "B1 12-project archive must fit a single physical flash sector");
static void zero(uint8_t *p,uint32_t n){while(n--)*p++=0u;}
static void put32(uint8_t *b,uint32_t n){for(unsigned i=0;i<4;i++)b[i]=(uint8_t)(n>>(8u*i));}
static uint32_t get32(const uint8_t *b){return (uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);}
uint32_t b1_storage_crc(const void *src,uint32_t n){
 const uint8_t *p=(const uint8_t*)src;uint32_t c=0xffffffffu;
 while(n--){c^=*p++;for(unsigned j=0;j<8;j++)c=(c>>1)^(0xedb88320u&-(c&1u));}
 return ~c;
}
uint8_t b1_storage_name_valid(const char name[B1_PROJECT_NAME_MAX+1u]){
 if(!name)return 0u;
 unsigned nonspace=0,terminated=0;
 for(unsigned i=0;i<=B1_PROJECT_NAME_MAX;i++){
  char c=name[i];
  if(!c){terminated=1;break;}
  if(!((c>='A'&&c<='Z')||(c>='0'&&c<='9')||c==' '||c=='-'||c=='_'||c=='.'))return 0u;
  if(c!=' ')nonspace++;
 }
 return (uint8_t)(terminated&&nonspace);
}
void b1_storage_default_name(uint8_t slot,char name[B1_PROJECT_NAME_MAX+1u]){
 static const char prefix[]="PROJECT ";unsigned i=0;
 while(prefix[i]){name[i]=prefix[i];i++;}
 name[i++]=(char)('0'+((slot+1u)/10u));name[i++]=(char)('0'+((slot+1u)%10u));
 name[i]=0;
 for(i++;i<=B1_PROJECT_NAME_MAX;i++)name[i]=0;
}
static int archive_valid(const struct b1_project_archive *a){
 if(a->mask & ~((1u<<B1_PROJECT_SLOTS)-1u))return 0;
 for(unsigned i=0;i<B1_PROJECT_SLOTS;i++)if(a->mask&(1u<<i)){
  if(!b1_storage_name_valid(a->names[i]))return 0;
  /* validate the embedded project's own checksum via its public helper. */
  if(!b1_project_valid(&a->projects[i]))return 0;
 }
 return 1;
}
#if !B1_STORAGE_DYNAMIC
static uint32_t base(unsigned bank){return bank?B1_STORAGE_B:B1_STORAGE_A;}
#endif
static int blank_sector(const struct b1_storage_io *io,uint32_t off){
 uint8_t buf[128];
 for(unsigned j=0;j<B1_STORAGE_SECTOR;j+=sizeof(buf)){
  if(io->read(io->ctx,off+j,buf,sizeof(buf)))return -1;
  for(unsigned k=0;k<sizeof(buf);k++)if(buf[k]!=0xffu)return 0;
 }
 return 1;
}
struct probe {uint8_t owned,valid,blank,committed;uint32_t gen;};
/* Main-loop-only storage workspace. Keeping the archive outside the small FM1
 * C stack prevents nested CRC/probe calls from overflowing the stack. */
static struct b1_project_archive b1_probe_workspace __attribute__((section(".pool")));
#if B1_STORAGE_DYNAMIC
static struct b1_project_archive b1_best_workspace __attribute__((section(".pool")));
#endif
/* V5 (schema 1) flash archive, immutable byte-layout compatibility.
 * Former struct b1_project was 112 bytes, checksum at offset 108.
 * New V6 expands fields; never mistake a valid V5 sector for foreign data. */
#define B1_V5_PROJECT_SIZE 112u
#define B1_V5_PROJECT_CHECKSUM 108u
#define B1_V5_ARCHIVE_LEN (208u+12u*B1_V5_PROJECT_SIZE)
static uint8_t b1_old_bytes[B1_V5_ARCHIVE_LEN] __attribute__((section(".pool")));
static uint32_t old_u32(const uint8_t *p){return get32(p);}
static uint32_t old_hash(const uint8_t *p){
 uint32_t h=2166136261u;
 for(uint32_t i=0;i<B1_V5_PROJECT_CHECKSUM;i++){h^=p[i];h*=16777619u;}
 return h;
}
static int upgrade_v5(struct b1_project_archive *dst,const uint8_t *old){
 zero((uint8_t*)dst,sizeof(*dst));
 uint32_t mask=old_u32(old);
 if(mask&~((1u<<B1_PROJECT_SLOTS)-1u))return 0;
 dst->mask=mask;
 for(uint32_t slot=0;slot<B1_PROJECT_SLOTS;slot++){
  for(uint32_t j=0;j<=B1_PROJECT_NAME_MAX;j++)dst->names[slot][j]=(char)old[4u+slot*17u+j];
  if(!(mask&(1u<<slot)))continue;
  if(!b1_storage_name_valid(dst->names[slot]))return 0;
  const uint8_t *src=old+208u+slot*B1_V5_PROJECT_SIZE;
  if(old_u32(src)!=0x31504242u||old_u32(src+4)!=5u||
     old_u32(src+B1_V5_PROJECT_CHECKSUM)!=old_hash(src))return 0;
  struct b1_project *d=&dst->projects[slot];
  uint8_t *to=(uint8_t*)d;
  for(uint32_t j=0;j<B1_V5_PROJECT_CHECKSUM;j++)to[j]=src[j];
  d->version=6u;
  d->delay_mix=0u;d->delay_time=45u;d->delay_feedback=30u;d->delay_spread=40u;
  d->reverb_type=0u;d->reverb_mix=0u;d->reverb_decay=50u;d->reverb_damp=60u;
  d->global_octave=4u;d->selected_bank=0u;
  uint32_t h=2166136261u;
  for(uint32_t j=0;j<sizeof(*d)-sizeof(d->checksum);j++){h^=to[j];h*=16777619u;}
  d->checksum=h;
  if(!b1_project_valid(d))return 0;
 }
 return 1;
}
static int probe_at(const struct b1_storage_io *io,uint32_t at,struct probe *q,
                    struct b1_project_archive *archive){
 uint8_t header[32];
 zero((uint8_t*)q,sizeof(*q));
 if(io->read(io->ctx,at,header,sizeof(header)))return -1;
 if(get32(header+HDR_MAGIC)!=B1_STORAGE_MAGIC){
  int blank=blank_sector(io,at);if(blank<0)return -1;q->blank=(uint8_t)blank;return 0;
 }
 q->owned=1u;q->committed=(uint8_t)(get32(header+HDR_COMMIT)==B1_STORAGE_COMMIT);
 if(get32(header+HDR_COMMIT)!=B1_STORAGE_COMMIT)return 0;
 uint32_t schema=get32(header+HDR_SCHEMA),len=get32(header+HDR_LEN);
 if(schema==1u && len==B1_V5_ARCHIVE_LEN){
  if(io->read(io->ctx,at+B1_STORAGE_DATA_OFFSET,b1_old_bytes,len))return -1;
  if(b1_storage_crc(b1_old_bytes,len)!=get32(header+HDR_CRC)||
     !upgrade_v5(&b1_probe_workspace,b1_old_bytes))return 0;
 }else if(schema==B1_STORAGE_SCHEMA && len==sizeof(struct b1_project_archive)){
  if(io->read(io->ctx,at+B1_STORAGE_DATA_OFFSET,(uint8_t*)&b1_probe_workspace,sizeof(b1_probe_workspace)))return -1;
  if(b1_storage_crc(&b1_probe_workspace,sizeof(b1_probe_workspace))!=get32(header+HDR_CRC)||!archive_valid(&b1_probe_workspace))return 0;
 }else return 0;
 q->valid=1u;q->gen=get32(header+HDR_GEN);
 if(archive)*archive=b1_probe_workspace;
 return 0;
}
#if !B1_STORAGE_DYNAMIC
static int probe_bank(const struct b1_storage_io *io,uint8_t bank,struct probe *q,
                      struct b1_project_archive *archive){
 return probe_at(io,base(bank),q,archive);
}
#endif
int b1_storage_open(struct b1_storage *s,const struct b1_storage_io *io){
 if(!s||!io||!io->read||!io->erase||!io->program)return -1;
 zero((uint8_t*)s,sizeof(*s));s->active_bank=-1;
#if B1_STORAGE_DYNAMIC
 /* Any version-1.6 B1 projects found in previously fixed sectors are
  * discovered automatically. Foreign sectors are NEVER used, even if one
  * of our own sectors remains valid. A second writable bank is essential. */
 uint32_t best=0u,spare=0u,gen=0u;
 uint8_t have=0u;
 /* Persistent scratch, never on the ISR stack. */
 for(uint32_t at=B1_STORAGE_SCAN_START;at<B1_STORAGE_SCAN_END;at+=B1_STORAGE_SECTOR){
  struct probe pr;
  if(probe_at(io,at,&pr,0)){s->state=B1_STORAGE_ERROR;return -2;}
  if(pr.valid && (!have || (int32_t)(pr.gen-gen)>0)){
   struct probe recheck;
   if(probe_at(io,at,&recheck,&b1_best_workspace)||!recheck.valid){s->state=B1_STORAGE_ERROR;return -2;}
   have=1u;gen=pr.gen;best=at;
  }
 }
 /* Choose the latest older VALID bank preferentially, otherwise a BLANK
  * sector, otherwise an interrupted B1-owned uncommitted bank. */
 uint32_t older=0u,older_gen=0u,empty=0u,interrupted=0u;
 uint8_t older_found=0u;
 for(uint32_t at=B1_STORAGE_SCAN_START;at<B1_STORAGE_SCAN_END;at+=B1_STORAGE_SECTOR){
  struct probe pr;
  if(probe_at(io,at,&pr,0)){s->state=B1_STORAGE_ERROR;return -2;}
  if(at==best&&have)continue;
  if(pr.valid && (!older_found || (int32_t)(pr.gen-older_gen)>0)){
   older_found=1u;older=at;older_gen=pr.gen;
  }
  if(pr.blank&&!empty)empty=at;
  if(pr.owned&&!pr.committed&&!interrupted)interrupted=at;
 }
 spare=older_found?older:(empty?empty:interrupted);
 if(have){
  s->archive=b1_best_workspace;s->generation=gen;s->sector[0]=best;
  s->sector[1]=spare;s->active_bank=0;
  s->state=spare?B1_STORAGE_READY:B1_STORAGE_LOCKED;
  return spare?1:-3;
 }
 if(empty){
  uint32_t second=0u;
  for(uint32_t at=B1_STORAGE_SCAN_START;at<B1_STORAGE_SCAN_END;at+=B1_STORAGE_SECTOR){
   if(at==empty)continue;
   struct probe pr;
   if(probe_at(io,at,&pr,0)){s->state=B1_STORAGE_ERROR;return -2;}
   if(pr.blank || (pr.owned&&!pr.committed)){second=at;break;}
  }
  if(second){s->sector[0]=empty;s->sector[1]=second;s->state=B1_STORAGE_EMPTY;return 0;}
 }
 s->state=B1_STORAGE_LOCKED;return -3;
#else
 struct probe a,b;struct b1_project_archive temp;
 s->sector[0]=B1_STORAGE_A;s->sector[1]=B1_STORAGE_B;
 if(probe_bank(io,0u,&a,0)||probe_bank(io,1u,&b,0)){s->state=B1_STORAGE_ERROR;return -2;}
 int chosen=-1;
 if(a.valid&&b.valid)chosen=((int32_t)(a.gen-b.gen)>0)?0:1;
 else if(a.valid)chosen=0;
 else if(b.valid)chosen=1;
 if(chosen>=0){
  struct probe check;
  if(probe_bank(io,(uint8_t)chosen,&check,&temp)||!check.valid){s->state=B1_STORAGE_ERROR;return -2;}
  s->archive=temp;s->generation=check.gen;s->active_bank=(int8_t)chosen;s->state=B1_STORAGE_READY;
  return 1;
 }
 /* A clean blank installation is allowed. Foreign data gets a hard lock. */
 if((a.blank&&b.blank) ||
    (a.owned&&!a.committed&&b.blank) ||
    (b.owned&&!b.committed&&a.blank)){
  /* Safe first-save recovery: reclaim only our own uncommitted journal or
   * use the remaining blank sector. Never discard a committed corrupt copy. */
  s->state=B1_STORAGE_EMPTY;return 0;
 }
 s->state=B1_STORAGE_LOCKED;return -3;
#endif
}
int b1_storage_commit(struct b1_storage *s,const struct b1_storage_io *io,
                      const struct b1_project_archive *candidate){
 if(!s||!io||!candidate||!archive_valid(candidate)||
    (s->state!=B1_STORAGE_READY&&s->state!=B1_STORAGE_EMPTY))return -1;
 uint8_t next=(uint8_t)(s->active_bank==0?1:0);
 uint32_t at=s->sector[next];
 if(at<B1_STORAGE_SCAN_START||at+B1_STORAGE_SECTOR>B1_STORAGE_SCAN_END ||
    (at & (B1_STORAGE_SECTOR-1u)))return -11;
 struct probe check; if(probe_at(io,at,&check,0))return -2;
 /* An unknown sector is NEVER erased, even when the other bank is ours. */
 if(!check.blank&&!check.owned)return -3;
 /* Do not erase a corrupted or unfamiliar committed B1 archive either. */
 if(check.committed&&!check.valid)return -12;
 if(!check.blank && io->erase(io->ctx,at))return -4;
 uint8_t header[32];for(unsigned i=0;i<sizeof(header);i++)header[i]=0xffu;
 uint32_t gen=s->generation+1u;
 put32(header+HDR_MAGIC,B1_STORAGE_MAGIC);
 put32(header+HDR_SCHEMA,B1_STORAGE_SCHEMA);
 put32(header+HDR_GEN,gen);
 put32(header+HDR_LEN,sizeof(struct b1_project_archive));
 put32(header+HDR_CRC,b1_storage_crc(candidate,sizeof(*candidate)));
 /* First mark target as B1-owned, then write data, finally commit. */
 if(io->program(io->ctx,at,header,sizeof(header)))return -5;
 const uint8_t *p=(const uint8_t*)candidate;
 for(uint32_t pos=0;pos<sizeof(*candidate);){
  uint32_t amount=sizeof(*candidate)-pos;if(amount>256u)amount=256u;
  if(io->program(io->ctx,at+B1_STORAGE_DATA_OFFSET+pos,p+pos,amount))return -6;
  pos+=amount;
 }
 if(io->read(io->ctx,at+B1_STORAGE_DATA_OFFSET,(uint8_t*)&b1_probe_workspace,sizeof(b1_probe_workspace)))return -7;
 if(b1_storage_crc(&b1_probe_workspace,sizeof(b1_probe_workspace))!=get32(header+HDR_CRC)||!archive_valid(&b1_probe_workspace))return -8;
 uint8_t final[4];put32(final,B1_STORAGE_COMMIT);
 if(io->program(io->ctx,at+HDR_COMMIT,final,4u))return -9;
 if(io->read(io->ctx,at+HDR_COMMIT,header,4u)||get32(header)!=B1_STORAGE_COMMIT)return -10;
 s->archive=*candidate;s->generation=gen;s->active_bank=(int8_t)next;s->state=B1_STORAGE_READY;
 return 0;
}

/* Reclaim only after a TWO-STEP physical confirmation, and only when B1
 * cannot find two usable sectors. It is impossible to identify who owns
 * arbitrary flash contents, so this NEVER claims to be a universal data
 * manager. This function is deliberately not reachable from OTA or ISR. */
int b1_storage_reclaim_confirmed(struct b1_storage *s,const struct b1_storage_io *io,
                                  uint8_t two_step_confirmed){
 if(!two_step_confirmed || !s || !io || !io->read || !io->erase ||
    !io->program || s->state!=B1_STORAGE_LOCKED || s->active_bank>=0)return -1;
 uint32_t first=0u,second=0u;
 /* The default reserved journal pair is chosen for locality, only after
  * the entire bounded region is checked to contain NO valid B1 archive.
  * This prevents corrupting a single surviving project copy. */
 for(uint32_t at=B1_STORAGE_SCAN_START;at<B1_STORAGE_SCAN_END;at+=B1_STORAGE_SECTOR){
  struct probe q;
  if(probe_at(io,at,&q,0))return -2;
  if(q.valid)return -3;
 }
 /* Prefer any blank sectors already available, then take the fewest
  * necessary foreign sectors. If a prior interrupted reclaim erased one
  * sector, retry can finish without erasing a third unknown sector. */
 for(uint32_t at=B1_STORAGE_SCAN_START;at<B1_STORAGE_SCAN_END;at+=B1_STORAGE_SECTOR){
  struct probe q;
  if(probe_at(io,at,&q,0))return -2;
  if(q.blank || (q.owned && !q.committed)){
   if(!first)first=at;else if(!second){second=at;break;}
  }
 }
 if(!second)for(uint32_t at=B1_STORAGE_SCAN_START;at<B1_STORAGE_SCAN_END;at+=B1_STORAGE_SECTOR){
  if(at==first)continue;
  struct probe q;
  if(probe_at(io,at,&q,0))return -2;
  /* Never reclaim a valid or corrupt committed B1 journal. */
  if(q.owned)return -4;
  if(!q.blank){if(!first)first=at;else if(!second){second=at;break;}}
 }
 if(!first||!second)return -5;
 /* The above range is explicitly fenced in both HAL and this module.
  * No BEETLE partition or updater sector can be reached through this API. */
 if(first<B1_STORAGE_SCAN_START||second+B1_STORAGE_SECTOR>B1_STORAGE_SCAN_END)return -6;
 struct probe q1,q2;
 if(probe_at(io,first,&q1,0)||probe_at(io,second,&q2,0))return -2;
 if(!q1.blank && io->erase(io->ctx,first))return -7;
 if(!q2.blank && io->erase(io->ctx,second))return -8;
 return b1_storage_open(s,io);
}
