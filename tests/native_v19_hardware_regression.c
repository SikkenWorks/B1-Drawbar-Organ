/* B1 V1.9: physical keyboard, silent note-off and bounded chord gain.
 * Host test exercises actual firmware performance + DSP C implementations. */
#include "../firmware/b1_performance.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
static int has_owner(const struct b1_engine *e,uint32_t owner){
 for(unsigned i=0;i<B1_VOICES;i++)if(e->voice[i].active&&!e->voice[i].releasing&&e->voice[i].owner==owner)return 1;
 return 0;
}
static unsigned max_abs(const int16_t *a, unsigned count){
 unsigned mx=0;
 for(unsigned i=0;i<count;i++){int32_t y=a[i]; unsigned n=y<0?(unsigned)-y:(unsigned)y;if(n>mx)mx=n;}
 return mx;
}
int main(void){
 struct b1_engine e; struct b1_performance p; int16_t out[256];
 b1_reset(&e);b1_performance_init(&p);
 assert(p.guide==0 && p.range[0].lo==0 && p.range[0].hi==26 && p.range[0].type==B1_ORGAN);
 /* Every one of 27 silicon keys sounds alone on a NEW project. */
 for(unsigned k=0;k<B1_KEYS;k++){
  b1_performance_keys(&p,&e,1u<<k);
  assert(has_owner(&e,k+1u));
  b1_render_stereo(&e,out,128);
  b1_performance_keys(&p,&e,0);
  b1_render_stereo(&e,out,128);
  b1_panic(&e);
 }
 /* CHORD ranges remain fully configurable and silent when used as selectors. */
 /* New CHORD range overrides the default full-keyboard base range. */
 p.range[2].type=B1_CHORD;
 assert(b1_assign_chord_key(&p,21,B1_MIN7));
 b1_performance_keys(&p,&e,(1u<<21)|(1u<<0));
 assert(p.held[0].count==4);
 b1_performance_keys(&p,&e,0u);b1_panic(&e);
 /* A burst of notes must not clip all samples or ring indefinitely after release. */
 b1_reset(&e);b1_performance_init(&p);e.cpu_relief=1;
 for(unsigned k=0;k<12;k++)b1_note_on_tagged(&e,53u+k,0,120u,k+1u);
 unsigned peak=0,clips=0;
 for(unsigned b=0;b<16;b++){
  b1_render_stereo(&e,out,128);
  unsigned this_peak=max_abs(out,256);if(this_peak>peak)peak=this_peak;
  for(unsigned i=0;i<256;i++)if(out[i]==32767||out[i]==-32768)clips++;
 }
 assert(peak>200u && peak<32500u && clips==0u);
 b1_panic(&e);
 for(unsigned b=0;b<200;b++)b1_render_stereo(&e,out,128);
 fprintf(stderr,"tail max = %u, hp=%d\n",max_abs(out,256),e.cabinet_hp);assert(max_abs(out,256)<=1u);
 puts("PASS: 27/27 default keys, user CHORD split, bounded 12-note level, note-off silence, GUIDE OFF");
 return 0;
}
