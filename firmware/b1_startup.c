/* SPDX-License-Identifier: GPL-3.0-only
 * Startup particle logo + centre-out physical LED ripple.
 * Deterministic fixed-point, no heap, no timers inside the renderer.
 * This runs before audio DMA starts; boot mode/USB update can bypass it.
 */
#include "b1_startup.h"
#include "b1_startup_masks.h"
#include <stdint.h>

#define B1_BOOT_BG 0x0841u
#define B1_BOOT_WHITE 0xEF7Du
#define B1_BOOT_PINK 0xEA78u
static uint16_t be16(uint16_t c){return (uint16_t)((c<<8)|(c>>8));}
static uint32_t mix_seed(uint32_t x){
 x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;return x;
}
static void pixel(uint16_t *f,int x,int y,uint16_t c,unsigned a){
 if((unsigned)x>=240u || (unsigned)y>=240u || a==0u)return;
 uint16_t *p=&f[y*240+x];
 if(a>=3u){*p=be16(c);return;}
 uint16_t o=be16(*p);
 unsigned r=(((o>>11)&31u)*(3u-a)+((c>>11)&31u)*a)/3u;
 unsigned g=(((o>>5)&63u)*(3u-a)+((c>>5)&63u)*a)/3u;
 unsigned b=((o&31u)*(3u-a)+(c&31u)*a)/3u;
 *p=be16((uint16_t)((r<<11)|(g<<5)|b));
}
static uint8_t mask_value(const uint8_t *bitmap,uint32_t n){return (uint8_t)((bitmap[n/4u]>>(6u-2u*(n%4u)))&3u);}
static void particles(uint16_t *f,const uint8_t *bmp,unsigned w,unsigned h,int x0,int y0,
                      uint32_t ms,uint32_t salt,uint16_t colour){
 for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
  uint32_t idx=y*w+x;uint32_t a=mask_value(bmp,idx);if(!a)continue;
  int dstx=x0+(int)x,dsty=y0+(int)y;
  if(ms>=B1_STARTUP_HOLD_BEGIN_MS){pixel(f,dstx,dsty,colour,a);continue;}
  uint32_t hsh=mix_seed((uint32_t)(dstx*509u+dsty*617u)+salt);
  uint32_t begin=105u+((hsh>>20)%255u);
  if(ms<begin)continue;
  uint32_t t=ms-begin;
  uint32_t p=t>=865u?256u:(t*256u)/865u;
  uint32_t ease=(p*p*(768u-2u*p)+32768u)>>16;
  int sx=(int)((hsh&0xffffu)%240u);
  int sy=(int)(((hsh>>8)^hsh)%240u);
  int px=sx+((dstx-sx)*(int)ease)/256;
  int py=sy+((dsty-sy)*(int)ease)/256;
  /* Thin, true 1-pixel particles. Only the text mask's own pixels move. */
  pixel(f,px,py,colour,a);
 }
}
static void fill(uint16_t *f,uint16_t colour){
 uint16_t c=be16(colour);
 for(uint32_t i=0;i<240u*240u;i++)f[i]=c;
}
static void darken(uint16_t *f,unsigned gain){
 if(gain>=256u)return;
 for(uint32_t i=0;i<240u*240u;i++){
  uint16_t c=be16(f[i]);unsigned r=((c>>11)&31u)*gain/256u;
  unsigned g=((c>>5)&63u)*gain/256u,b=(c&31u)*gain/256u;
  f[i]=be16((uint16_t)((r<<11)|(g<<5)|b));
 }
}
void b1_startup_render(uint16_t *frame,const struct b1_ui_state *home,uint32_t ms){
 if(ms>=3030u){
  b1_ui_render(frame,home);
  if(ms<3220u)darken(frame,(ms-3030u)*256u/190u);
  return;
 }
 fill(frame,B1_BOOT_BG);
 /* Full-size mask pixels are the final stable glyphs, assembled from randomly
  * scattered points. Inter glyph bitmaps are part of firmware source. */
 particles(frame,b1_boot_mask0,B1_BOOT_MASK0_W,B1_BOOT_MASK0_H,
           B1_BOOT_MASK0_X,B1_BOOT_MASK0_Y,ms,0x51334297u,B1_BOOT_WHITE);
 particles(frame,b1_boot_mask1,B1_BOOT_MASK1_W,B1_BOOT_MASK1_H,
           B1_BOOT_MASK1_X,B1_BOOT_MASK1_Y,ms,0x87572423u,B1_BOOT_WHITE);
 if(ms>=1300u){
  /* Tiny centred accent: intentional 22-pixel brand mark, no animated noise. */
  for(int x=109;x<131;x++)pixel(frame,x,129,B1_BOOT_PINK,2);
 }
 if(ms>=2900u)darken(frame,(3030u-ms)*256u/130u);
}
/* Real FM1 physical indices: 0..13 buttons; 14..40 note LEDs.
 * Keys 13 and 14 are the two centre triggers. One concentric wave proceeds
 * along keyboard; function buttons join as the expanding wave reaches their
 * respective left and right control clusters. Binary brightness on stock HAL.
 */
static const uint16_t function_arrival[14]={
 830,735,470,540,610,680,750,825,500,570,640,710,780,855
};
void b1_startup_leds(uint32_t ms,void (*set_led)(uint32_t,int)){
 for(uint32_t id=0;id<41u;id++){
  uint32_t arrival;
  if(id<14u)arrival=function_arrival[id];
  else{
   uint32_t k=id-14u;
   uint32_t d=k<=13u?13u-k:k-14u;
   arrival=200u+d*67u;
  }
  int on=(ms>=arrival && ms<arrival+138u);
  set_led(id,on);
 }
}
