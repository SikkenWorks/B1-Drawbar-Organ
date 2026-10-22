/* Native packet-level replay of the production ota.c. Does NOT flash hardware. */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define FELUCCA_ID "FM-1_2147"
#define FELUCCA_OTA_DRYRUN 0
#include "ota.c"
static uint8_t *package;static uint32_t package_len;
static uint8_t stage[OTA_AREA_LEN], protected_boot[0x4000];
static uint8_t response[700];static uint32_t response_len;
static uint32_t ms, reads, erases, writes, commits, special_requests;
static int deny_success, cancelled, io_failure, readback_corrupt;
static uint8_t committed[112];
static void reply(uint32_t addr,uint32_t size){
 uint8_t dec[560];
 dec[0]=0;dec[1]=0x59;dec[2]=0x30;dec[3]=(uint8_t)(size+8);dec[4]=(uint8_t)((size+8)>>8);dec[5]=0;
 dec[6]=0;
 ota_wr32(dec+7,addr);
 dec[11]=(uint8_t)size;dec[12]=(uint8_t)(size>>8);dec[13]=0;
 if(addr==0xE0000000u){
   ++special_requests;memcpy(dec+14,deny_success?"notready":"success",size);
 }else if(addr<=package_len && size<=package_len-addr){
   memcpy(dec+14,package+addr,size);
 }else{response_len=0;return;}
 uint32_t sum=0;for(uint32_t i=6;i<14+size;i++)sum+=dec[i];dec[14+size]=(uint8_t)~sum;
 response_len=ota_pack7(dec,15+size,response);
}
static int ota_wire_send(const uint8_t *p,uint32_t n){
 if(n<3||p[0]!=0xf0||p[n-1]!=0xf7)return -1;
 uint8_t msg[80];uint32_t d=ota_unpack7(p+1,n-2,msg,sizeof msg);
 if(d>=15&&msg[2]==0x30){uint32_t addr=ota_rd32(msg+7);uint32_t len=ota_rd16(msg+11);
   if(len>512u){fprintf(stderr,"bad request %u\n",len);exit(3);}reads++;reply(addr,len);
 }
 return 0;
}
static int ota_frame_get(const uint8_t **p,uint32_t *n){if(!response_len)return 0;*p=response;*n=response_len;return 1;}
static void ota_frame_done(void){response_len=0;}
static uint32_t ota_now_ms(void){return ms;}
static void ota_idle(void){ms++;}
static int ota_aborted(void){return cancelled && ms>50u;}
static int ota_erase(uint32_t off){
 if(off<OTA_AREA||off>=OTA_AREA+OTA_AREA_LEN||(off&0xfffu))return -1;
 if(io_failure&&erases>=2u)return -1;
 memset(stage+off-OTA_AREA,255,0x1000);erases++;return 0;
}
static int ota_prog(uint32_t off,const void *p,uint32_t n){
 if(off<OTA_AREA||off>=OTA_AREA+OTA_AREA_LEN||n>OTA_AREA+OTA_AREA_LEN-off)return -1;
 for(uint32_t i=0;i<n;i++){
   uint8_t a=stage[off-OTA_AREA+i],v=((const uint8_t *)p)[i];
   if((a&v)!=v)return -1;
   stage[off-OTA_AREA+i]=v;
 }
 writes++;return 0;
}
static int ota_fread(uint32_t off,void *p,uint32_t n){
 if(off<0x4000u&&n<=0x4000u-off){memcpy(p,protected_boot+off,n);return 0;}
 if(off>=OTA_AREA&&off<=OTA_AREA+OTA_AREA_LEN&&n<=OTA_AREA+OTA_AREA_LEN-off){memcpy(p,stage+off-OTA_AREA,n);if(readback_corrupt && writes>2u && off>=OTA_AREA+32u && off<OTA_AREA+0x2000u) ((uint8_t*)p)[0]^=0x10u;return 0;}
 return -1;
}
static void ota_show(uint32_t step,int32_t code){(void)step;(void)code;}
static void ota_commit(const uint8_t *p){memcpy(committed,p,112u);commits++;}
int main(int argc,char **argv){
 if(argc<4){fprintf(stderr,"usage: %s LOGICAL_UFW BOOT_0x4000 expect_pass|expect_reject [no-success|cancel|erase-fail]\n",argv[0]);return 3;}
 FILE *f=fopen(argv[1],"rb");if(!f)return 3;
 fseek(f,0,SEEK_END);long ln=ftell(f);rewind(f);
 if(ln<0||ln>0x200000){fclose(f);return 3;}package_len=(uint32_t)ln;package=malloc(package_len);
 if(fread(package,1,package_len,f)!=package_len)return 3;fclose(f);
 f=fopen(argv[2],"rb");if(!f)return 3;
 if(fread(protected_boot,1,sizeof protected_boot,f)!=sizeof protected_boot)return 3;fclose(f);
 memset(stage,255,sizeof stage);
 int variant_expected=argc>4 && !strcmp(argv[4],"incoming");
 if(argc>4){deny_success=!strcmp(argv[4],"no-success");cancelled=!strcmp(argv[4],"cancel");io_failure=!strcmp(argv[4],"erase-fail");readback_corrupt=!strcmp(argv[4],"readback-corrupt");}
 int rc=ota_session();int pass=!strcmp(argv[3],"expect_pass"), prereject=!strcmp(argv[3],"expect_prereject");
 int boot_ok=1;/* boot[] was never writable by this harness */
 if(pass&&variant_expected&&rc==0){
   /* The staged writer must be the complete validated external body, not
    * merely the old pinned loader that caused cross-firmware limitations. */
   uint8_t hdr[1024];uint32_t flo,fln,olo,oln;
   if(ota_ufw(hdr,&flo,&fln,&olo,&oln)!=0||oln<32u||
      ota_rd32(stage+8)!=oln||memcmp(stage+32,package+olo+32,oln-32u))boot_ok=0;
 }
 int known_loader_ok=(variant_expected || !memcmp(stage+32,b1_trusted_loader,ETCH_OTA_BODY_LEN));
 int ok=(pass?(rc==0&&commits==1&&special_requests>=1 && erases==5u && writes>=2u && known_loader_ok && !memcmp(committed+8,variant_expected?"ota-FM-1_900":"ota-FM-1_015",12)):
              (rc!=0&&commits==0&&(!prereject||(erases==0&&writes==0&&special_requests==0))))&&boot_ok;
 printf("%s %s rc=%d reads=%u special=%u erases=%u writes=%u commits=%u ms=%u stageId=%.*s\n",
   ok?"PASS":"FAIL",argv[1],rc,reads,special_requests,erases,writes,commits,ms,12,committed+8);
 free(package);return ok?0:1;
}
