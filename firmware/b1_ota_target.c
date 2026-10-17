/* GPL-3.0-only. B1 hardware OTA adapter based on Felucca / BEETLE. */
/* This file is included after ota.c from b1_main.c. */
static uint32_t ota_now_ms(void){return b1_ms;}
static int ota_aborted(void){return usb.uboot_req || (fm1_in.buttons & 3u)==3u;}
static void ota_idle(void){fm1_wdt_feed();usb_poll();}
static int ota_erase(uint32_t off){
 if(!b1_updating || !FL_IN(off,4096u,OTA_AREA,OTA_AREA+OTA_AREA_LEN) || (off&4095u))return -8;
 uint32_t took;fm1_irq_off();int r=FL_FAR(fl_erase4k_ram)(off,&took);fm1_irq_enable_all();fm1_wdt_feed();return r;
}
static int ota_prog(uint32_t off,const void *src,uint32_t n){
 if(!b1_updating || !FL_IN(off,n,OTA_AREA,OTA_AREA+OTA_AREA_LEN))return -8;
 const uint8_t *p=src;uint8_t page[256];
 while(n){uint32_t k=256u-(off&255u);if(k>n)k=n;
   for(uint32_t j=0;j<k;j++)page[j]=p[j];
   uint32_t took;fm1_irq_off();int r=FL_FAR(fl_prog_ram)(off,page,k,&took);fm1_irq_enable_all();fm1_wdt_feed();
   if(r)return r;off+=k;p+=k;n-=k;
 }return 0;
}
static int ota_fread(uint32_t off,void *dst,uint32_t n){
 if(!b1_updating || !(FL_IN(off,n,0,0x4000u)||FL_IN(off,n,OTA_AREA,OTA_AREA+OTA_AREA_LEN)))return -8;
 uint8_t *p=dst;
 while(n){uint32_t k=n>256u?256u:n;fm1_irq_off();int r=FL_FAR(fl_read_ram)(off,p,k);fm1_irq_enable_all();fm1_wdt_feed();if(r)return r;off+=k;p+=k;n-=k;}return 0;
}
static void ota_show(uint32_t step,int32_t code){
 b1_ota_step=(uint8_t)step;b1_ota_error=code;
 b1_screen();
}
static void ota_commit(const uint8_t *parm){
 if(!b1_updating)return;
 usb_detach();fm1_delay_ms(30);fm1_irq_off();fm1_enter_update(parm);
}
