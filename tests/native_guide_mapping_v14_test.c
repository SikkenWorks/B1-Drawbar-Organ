/* SPDX-License-Identifier: GPL-3.0-only
 * Physical keyboard (F3..G5) scale-guide regression, all 27 keys.
 * Exercises exactly the production b1_apply_leds and b1_note_allowed functions.
 */
#include "../firmware/b1_led_policy.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static int observed[41],calls;
static void led(uint32_t id,int value){assert(id<41);assert(observed[id]==-1);observed[id]=!!value;calls++;}
static void refresh(const struct b1_performance *p,uint32_t pressed,uint8_t page){
 for(unsigned i=0;i<41;i++)observed[i]=-1;
 calls=0;b1_apply_leds(p,pressed,page,led);
 assert(calls==41);
 for(unsigned i=0;i<41;i++)assert(observed[i]>=0);
}
static void compare_physical(const struct b1_performance *p){
 refresh(p,0,5);
 for(uint8_t k=0;k<27;k++){
  uint8_t midi=(uint8_t)(B1_KEYBOARD_LOW_MIDI+k);
  int expected=p->guide && b1_note_allowed(p,midi);
  if(observed[14+k]!=expected){
   fprintf(stderr,"Guide mismatch key=%u MIDI=%u root=%u scale=%u: got %d expected %d\n",k,midi,p->scale_root,p->scale_type,observed[14+k],expected);
   assert(0);
  }
 }
 for(int b=0;b<14;b++)assert(observed[b]==(b==7));
}
int main(void){
 struct b1_performance p;
 b1_performance_init(&p);
 /* Default range 20..26 is CHORD: the previous implementation illuminated
    all seven chord-selector LEDs, including C#/D#/F# on C major. */
 /* Simulate a user-enabled CHORD range; LEDs remain scale-only. */
 p.range[0].hi=19;p.range[2].type=B1_CHORD;
 assert(p.range[2].type==B1_CHORD && p.range[2].lo==20 && p.range[2].hi==26);
 p.scale_root=0;p.scale_type=1;p.guide=1;
 compare_physical(&p);
 static const uint8_t naturals[12]={1,0,1,0,1,1,0,1,0,1,0,1};
 for(uint8_t k=0;k<27;k++)assert(observed[14+k]==naturals[(53u+k)%12u]);
 assert(!observed[14+20]); /* C#5, chord key: must NOT light */
 assert(!observed[14+22]); /* D#5, chord key: must NOT light */
 assert(!observed[14+25]); /* F#5, chord key: must NOT light */
 /* Tonality: ALL 12 roots x ALL 4 built-in scales x ALL 27 keys. */
 for(uint8_t root=0;root<12;root++)for(uint8_t scale=0;scale<B1_SCALE_COUNT;scale++){
  p.scale_root=root;p.scale_type=scale;compare_physical(&p);
 }
 /* Changing ranges/keyboard octave/CHORD assignment never changes guide. */
 p.scale_root=0;p.scale_type=1;
 refresh(&p,0,5);int baseline[41];memcpy(baseline,observed,sizeof(baseline));
 p.range[0].type=B1_CHORD;p.range[1].type=B1_DISABLED;p.range[2].type=B1_ORGAN;
 p.global_octave=7;p.range[2].octave=-4;
 refresh(&p,0,5);assert(!memcmp(baseline,observed,sizeof(baseline)));
 /* KEYLOCK should change pitch quantization, not the visual scale guide. */
 p.keylock=1;refresh(&p,0,5);assert(!memcmp(baseline,observed,sizeof(baseline)));
 /* Pressed off-scale notes still illuminate momentarily, without changing others. */
 refresh(&p,1u<<20,5);assert(observed[14+20]==1);
 for(int b=0;b<14;b++)assert(observed[b]==baseline[b]);
 p.guide=0;refresh(&p,0,5);
 for(uint8_t k=0;k<27;k++)assert(observed[14+k]==0);
 /* Function LEDs must remain page-only, not affected by the note guide. */
 p.guide=1;refresh(&p,0,2);
 for(int b=0;b<14;b++)assert(observed[b]==(b==4));
 printf("B1 V1.4: C major chord-key regression, 48 scales, 1296 physical LED comparisons, 41 LED independence PASS\n");
 return 0;
}
