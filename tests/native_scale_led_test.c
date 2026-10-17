/* SPDX-License-Identifier: GPL-3.0-only
 * Regression: scale names, roots and physical FM1 LED addressing.
 * This test uses the actual UI renderer and the same LED policy as firmware.
 */
#include "../firmware/b1_ui.h"
#include "../firmware/b1_led_policy.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t leds[41];
static int calls;
static uint16_t framebuffer[240u*240u];
static uint16_t previous[240u*240u];
static void led(uint32_t id,int on){ assert(id<41u);assert(leds[id]==255u);leds[id]=(uint8_t)!!on;calls++; }
static void refresh(const struct b1_performance *p,uint32_t notes,uint8_t page){
 memset(leds,255,sizeof(leds));calls=0;
 b1_apply_leds(p,notes,page,led);
 assert(calls==41);
 for(int i=0;i<41;i++)assert(leds[i]!=255u);
}
static void verify_control_independence(struct b1_performance *p){
 uint8_t baseline[14];
 p->guide=0;p->scale_root=0;p->scale_type=0;
 refresh(p,0,5);memcpy(baseline,leds,14);
 for(unsigned r=0;r<12;r++)for(unsigned sc=0;sc<B1_SCALE_COUNT;sc++){
  p->scale_root=(uint8_t)r;p->scale_type=(uint8_t)sc;p->guide=1;
  refresh(p,0,5);
  assert(!memcmp(leds,baseline,14));
 }
 /* The GUIDE can illuminate only the correct piano LEDs 14..40. */
 p->guide=0;refresh(p,(1u<<0)|(1u<<26),5);
 assert(leds[14]==1 && leds[40]==1 && leds[15]==0);
 assert(!memcmp(leds,baseline,14));
 p->guide=1;refresh(p,0,1); /* Page navigation lights FX only. */
 for(int i=0;i<14;i++)assert(leds[i]==(i==2));
 refresh(p,0,0);for(int i=0;i<14;i++)assert(leds[i]==(i==8));
}
static int differs_in_area(const uint16_t *a,const uint16_t *b,int x0,int y0,int x1,int y1){
 for(int y=y0;y<y1;y++)for(int x=x0;x<x1;x++)if(a[y*240+x]!=b[y*240+x])return 1;
 return 0;
}
int main(void){
 static const char *const roots[12]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
 static const char *const scale_names[4]={"CHROMATIC","MAJOR","MINOR","LYDIAN"};
 for(int i=0;i<12;i++)assert(!strcmp(b1_root_name((uint8_t)i),roots[i]));
 for(int i=0;i<4;i++)assert(!strcmp(b1_scale_name((uint8_t)i),scale_names[i]));
 struct b1_engine engine;struct b1_performance p;
 b1_performance_init(&p);b1_reset(&engine);
 verify_control_independence(&p);
 /* Rendering visibly changes RIGHT-HAND root/scale labels but keeps their
  * background and LEFT-HAND labels stable. The real glyph drawing is used. */
 struct b1_ui_state ui={0};ui.audio=&engine;ui.keys=&p;ui.page=5;
 p.scale_root=0;p.scale_type=0;
 b1_ui_render(previous,&ui);
 p.scale_root=11;p.scale_type=3;
 b1_ui_render(framebuffer,&ui);
 assert(differs_in_area(previous,framebuffer,110,83,225,109));
 assert(differs_in_area(previous,framebuffer,110,117,225,144));
 for(int y=82;y<144;y++)for(int x=0;x<80;x++)assert(previous[y*240+x]==framebuffer[y*240+x]);
 /* Preserve the V1.7 indices and their exact first four musical masks. */
 p.scale_root=0;
 static const uint8_t expected[4][12]={
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,1,0,1,1,0,1,0,1,0,1},
  {1,0,1,1,0,1,0,1,1,0,1,0},
  {1,0,1,0,1,0,1,1,0,1,0,1}
 };
 for(uint8_t sc=0;sc<4u;sc++){
  p.scale_type=sc;
  for(uint8_t note=0;note<12;note++)assert(b1_note_allowed(&p,note)==expected[sc][note]);
 }
 puts("B1 V1.1 ROOT/SCALE names, masks, 41 LED IDs, page-independence and real UI renderer: PASS");
 return 0;
}
