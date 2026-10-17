/* GPL-3.0-only. Validate actual B1 firmware renderer travel: 0 top, 8 bottom. */
#include "../firmware/b1_ui.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static uint16_t frame[240*240];
static uint16_t pixel(int x,int y){ uint16_t c=frame[y*240+x];return (uint16_t)((c>>8)|(c<<8)); }
int main(void){
 static struct b1_engine e;
 static struct b1_performance p;
 struct b1_ui_state s={0};
 b1_reset(&e);b1_performance_init(&p);s.audio=&e;s.keys=&p;
 p.selected_bank=0;s.page=0;s.drawbar_page=0;
 for(int v=0;v<=8;v++){
  e.drawbars[0][0]=(uint8_t)v;
  b1_ui_render(frame,&s);
  /* Main handle: column 20 lies inside handle and outside 9 px slider track. */
  assert(pixel(20,145+v*6)==0xFD40u);
  if(v!=4)assert(pixel(20,145+(8-v)*6)!=0xFD40u);
  /* Mini strip extends downwards, never grows upward from its base. */
  if(v==0)assert(pixel(19,84)!=0xFD40u);
  if(v>0){ assert(pixel(19,84)==0xFD40u); assert(pixel(19,83+2*v)==0xFD40u); }
  if(v<8)assert(pixel(19,84+2*v)!=0xFD40u);
 }
 puts("Native B1 drawbars: levels 0..8 move TOP -> BOTTOM; miniature matches: PASS");
 return 0;
}
