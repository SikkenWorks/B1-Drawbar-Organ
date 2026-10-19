/* GPL-3.0-only. Physical 41-LED KEY ASSIGN / SCALE GUIDE integration tests.
 * This calls production b1_apply_leds() and live b1_performance.c selectors.
 */
#include "../firmware/b1_led_policy.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static int state[41],count;
static struct b1_engine engine;
static void emit_led(uint32_t id,int on){assert(id<41u);assert(state[id]==-1);state[id]=!!on;count++;}
static void draw(const struct b1_performance *p,uint8_t page,uint32_t down){
 for(unsigned i=0;i<41u;i++)state[i]=-1;
 count=0;b1_apply_leds(p,down,page,emit_led);
 assert(count==41);
 for(unsigned i=0;i<41u;i++)assert(state[i]>=0);
 for(unsigned i=0;i<14u;i++)assert(state[i]==(i==b1_led_page_button(page)));
}
static void assert_chord_lit(const struct b1_performance *p,uint8_t page,uint32_t down){
 draw(p,page,down);
 for(uint8_t k=0;k<B1_KEYS;k++)assert(state[14u+k]==(!!(down&(1u<<k)) || !!b1_chord_selector_key(p,k)));
}
int main(void){
 struct b1_performance p;
 b1_performance_init(&p);
 b1_reset(&engine);
 /* Default project has no active CHORD keys, even though its chord map
  * already contains default qualities for keys 21-27. */
 assert(!b1_chord_enabled(&p));
 draw(&p,6,0);
 for(int k=0;k<27;k++)assert(state[14+k]==0);
 assert(!b1_chord_selector_key(&p,20u) && !b1_chord_selector_key(&p,27u));
 /* Enabling K3 assigns active range 20..26. All seven are steadily lit. */
 assert(b1_set_chord_enabled(&p,&engine,1));
 assert(b1_chord_enabled(&p));
 assert_chord_lit(&p,6,0);
 for(uint8_t k=0;k<27u;k++)assert(state[14u+k]==(k>=20u));
 /* Remain lit on every performance page; normal notes stay off at idle. */
 for(uint8_t page=0;page<5u;page++)assert_chord_lit(&p,page,0);
 assert_chord_lit(&p,6,0);
 /* Button LEDs remain independent; pressing ORGAN keys still gives feedback. */
 assert_chord_lit(&p,0,(1u<<0)|(1u<<13)|(1u<<25));
 assert(state[14] && state[14+13] && state[14+25] && state[14+26]);
 assert_chord_lit(&p,0,0);
 assert(!state[14] && !state[14+13]);
 /* Quality values alone do not light other keys, including accidental saves. */
 p.chord_map[7]=B1_MAJ9;
 assert(!b1_chord_selector_key(&p,7));
 assert(b1_assign_chord_key(&p,21u,B1_MIN7));
 assert(p.chord_map[21u]==B1_MIN7);
 assert(!b1_assign_chord_key(&p,7u,B1_DOM7));
 assert_chord_lit(&p,6,0);
 /* SCALE GUIDE takes priority ON THE SCALE PAGE and uses true chromatic pitch. */
 p.guide=1u;p.scale_root=0u;p.scale_type=1u;
 draw(&p,5,0);
 for(uint8_t k=0;k<B1_KEYS;k++){
  int expected=b1_guide_key_allowed(&p,k);
  assert(state[14u+k]==expected);
 }
 /* CHORD overrides GUIDE on other pages: ordinary scale tones stay dark. */
 assert_chord_lit(&p,0,0);
 assert_chord_lit(&p,6,0);
 /* Higher-priority ORGAN ranges hide overlapping CHORD keys as note routing. */
 p.range[3]=(struct b1_range){24,26,B1_ORGAN,0,0,0};
 assert_chord_lit(&p,6,0);
 for(uint8_t k=0;k<27u;k++)assert(state[14u+k]==(k>=20u && k<=23u));
 /* DISABLED ranges do not shadow lower ranges in existing B1 note routing;
  * the indicator must follow that same established priority. */
 p.range[3].type=B1_DISABLED;
 assert(b1_chord_selector_key(&p,24));
 /* Disable the actual CHORD range: no keys may be reported as selectors. */
 p.range[2].type=B1_DISABLED;
 assert(!b1_chord_enabled(&p));
 draw(&p,6,0);
 for(int k=0;k<27;k++)assert(state[14+k]==0);
 /* Re-activate and confirm project capture/apply preserves exact LED roles. */
 p.range[2].type=B1_CHORD;
 p.range[3].type=B1_ORGAN;
 struct b1_project pr;
 b1_project_capture(&pr,&p,&engine);
 struct b1_performance restored;
 struct b1_engine restored_engine;
 b1_performance_init(&restored);b1_reset(&restored_engine);
 assert(b1_project_apply(&pr,&restored,&restored_engine));
 assert(restored.chord_map[21]==B1_MIN7);
 assert_chord_lit(&restored,6,0);
 for(uint8_t k=0;k<27u;k++)assert(state[14u+k]==(k>=20u && k<=23u));
 /* Explicit ASSIGN OFF retains saved chord data without displaying triggers. */
 assert(b1_set_chord_enabled(&restored,&restored_engine,0));
 assert(restored.chord_map[21]==B1_MIN7);
 draw(&restored,6,0);
 for(int k=0;k<27;k++)assert(state[14+k]==0);
 /* When not editing and assignment OFF, original GUIDE is still intact. */
 draw(&restored,0,0);
 for(uint8_t k=0;k<27u;k++)assert(state[14u+k]==b1_guide_key_allowed(&restored,k));
 /* Legacy project mode=0 auto-detect CHORD ranges, including overlaps. */
 restored.chord_mode=0;
 assert_chord_lit(&restored,6,0);
 for(uint8_t k=0;k<27u;k++)assert(state[14u+k]==(k>=20u && k<=23u));
 /* Invalid key references must never overflow 41-LED hardware matrix. */
 assert(!b1_chord_selector_key(&restored,B1_KEYS));
 assert(!b1_chord_selector_key(0,0));
 puts("PASS V4.0: KEY ASSIGN persistent LEDs, 41 hardware IDs, scale guide, overlaps, save/load, press feedback, OFF and legacy projects");
 return 0;
}
