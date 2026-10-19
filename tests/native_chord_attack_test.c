/* Native sample-timing regression for B1 V0.9 chord ATTACK. */
#include "../firmware/b1_performance.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned active_for(const struct b1_engine *e,unsigned owner){
 unsigned total=0;
 for(unsigned i=0;i<B1_VOICES;i++)if(e->voice[i].active&&e->voice[i].owner==owner&&!e->voice[i].releasing)total++;
 return total;
}
static unsigned count_pending(const struct b1_held *h){
 unsigned n=0;for(unsigned i=0;i<h->count;i++)n+=h->pending[i];return n;
}
int main(void){
 struct b1_engine e;struct b1_performance p;
 int16_t sound[256];
 b1_reset(&e);b1_performance_init(&p);
 p.range[0].hi=19;p.range[2].type=B1_CHORD;
 assert(p.range[2].attack==0);
 /* Modifier 21 = MIN, note 1 => F# minor; 0 must be simultaneous. */
 b1_performance_keys(&p,&e,(1u<<21)|(1u<<1));
 assert(p.held[1].count==3&&count_pending(&p.held[1])==0);
 assert(active_for(&e,2)==3);
 b1_performance_keys(&p,&e,0);
 /* Independent range attack should not affect standalone organ notes. */
 p.range[2].attack=100;
 b1_performance_keys(&p,&e,1u<<1);assert(active_for(&e,2)==1);
 assert(count_pending(&p.held[1])==0);
 b1_performance_keys(&p,&e,0);
 b1_panic(&e);
 /* With attack set high, the earliest voice starts immediately and some wait. */
 unsigned delayed=0,unordered=0,variations=0;
 uint32_t first_pattern=0;
 for(unsigned t=0;t<18;t++){
  b1_performance_keys(&p,&e,(1u<<21)|(1u<<1));
  struct b1_held *h=&p.held[1];
  assert(h->count==3);
  unsigned pending=count_pending(h);
  assert(pending<3);
  if(pending){
   delayed++;
   uint32_t vals[3];for(unsigned i=0;i<3;i++)vals[i]=h->due[i]-p.clock;
   if(vals[0]>vals[1]||vals[1]>vals[2])unordered++;
   uint32_t pattern=vals[0]*13u+vals[1]*37u+vals[2]*83u;
   if(t==0)first_pattern=pattern;else if(pattern!=first_pattern)variations++;
   assert(active_for(&e,2)>=1);
   b1_performance_render(&p,&e,sound,128); /* spans events sample accurately */
   assert(p.clock>=128u);
   for(unsigned i=0;i<h->count;i++)if(h->pending[i])assert((int32_t)(h->due[i]-p.clock)>0);
  }
  /* fast release should cancel notes that are still pending */
  b1_performance_keys(&p,&e,0);
  assert(p.held[1].count==0&&count_pending(&p.held[1])==0);
  b1_performance_render(&p,&e,sound,128);
  assert(active_for(&e,2)==0);
  b1_panic(&e);
 }
 assert(delayed>=15&&unordered>2&&variations>12);
 /* One chord must finish correctly over multiple 128-sample blocks. */
 b1_performance_keys(&p,&e,(1u<<21)|(1u<<1));
 unsigned tries=0;
 while(count_pending(&p.held[1])&&tries++<32)b1_performance_render(&p,&e,sound,128);
 assert(count_pending(&p.held[1])==0&&active_for(&e,2)==3);
 b1_performance_keys(&p,&e,0);b1_panic(&e);
 /* 2 different chord groups with distinct ATTACK settings. */
 p.range[3]=(struct b1_range){22u,26u,B1_CHORD,0u,0,0};
 p.range[2].hi=21u;p.range[2].attack=100u;
 b1_performance_keys(&p,&e,(1u<<22)|(1u<<1));
 assert(p.held[1].count==4&&count_pending(&p.held[1])==0);
 b1_performance_keys(&p,&e,0);
 /* Project V4 persists attack, validates bounds, and doesn't modify others. */
 p.range[2].attack=64;
 struct b1_project save;b1_project_capture(&save,&p,&e);
 assert(save.version==6&&save.ranges[2].attack==64);
 p.range[2].attack=0;assert(b1_project_apply(&save,&p,&e));assert(p.range[2].attack==64);
 struct b1_project corrupt=save;corrupt.ranges[2].attack=101;
 /* Capture a consistent checksum for an invalid range: applying must reject it. */
 struct b1_performance tp=p;tp.range[2].attack=101;
 b1_project_capture(&corrupt,&tp,&e);
 assert(!b1_project_apply(&corrupt,&p,&e));
 puts("B1 V0.9 ATTACK: simultaneous zero, random order, sample scheduling, key-up cancel, range independence and V4 storage PASS");
 return 0;
}
