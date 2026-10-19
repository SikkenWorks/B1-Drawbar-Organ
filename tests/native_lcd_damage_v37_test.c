/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "b1_lcd_dirty.h"
static uint16_t frame[B1_LCD_WIDTH*B1_LCD_HEIGHT];
static struct b1_lcd_damage state;
static uint8_t dirty[B1_LCD_HEIGHT];
static uint32_t rows_covered(void){
 uint32_t cursor=0, start, count, seen=0;
 while(b1_lcd_next_damage(dirty,&cursor,&start,&count)){
  assert(start<B1_LCD_HEIGHT && count && count<=B1_LCD_MAX_STRIP);
  assert(start+count<=B1_LCD_HEIGHT);
  for(uint32_t y=start;y<start+count;y++)if(dirty[y])++seen;
 }
 return seen;
}
int main(void){
 assert(b1_lcd_detect_damage(&state,frame,dirty)==240);
 assert(rows_covered()==240);
 assert(b1_lcd_detect_damage(&state,frame,dirty)==0);
 frame[77*240+32] = 0xbeefu;
 assert(b1_lcd_detect_damage(&state,frame,dirty)==1);
 assert(dirty[77] && rows_covered()==1);
 frame[78*240+32] = 0x1234;
 frame[81*240+50] = 0x4321;
 frame[200*240+20] = 5;
 assert(b1_lcd_detect_damage(&state,frame,dirty)==3);
 assert(rows_covered()==3);
 assert(b1_lcd_detect_damage(&state,frame,dirty)==0);
 for(unsigned i=0;i<90;i++){
  uint32_t count=b1_lcd_detect_damage(&state,frame,dirty);
  if(count) {assert(count==240);assert(rows_covered()==240);}
 }
 /* Worst case alternating rows never generates a >115 row SPI strip. */
 for(unsigned y=0;y<240;y+=2)frame[y*240+y/2]^=1;
 assert(b1_lcd_detect_damage(&state,frame,dirty)==120);
 assert(rows_covered()==120);
 puts("PASS: V3.7 LCD row-damage detection, full refresh, bounded spans");
 return 0;
}
