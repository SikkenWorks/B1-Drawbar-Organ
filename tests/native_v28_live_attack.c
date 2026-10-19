/* V2.8 live, audible chord timing: zero simultaneous, nonzero spread,
 * cross-block release/cancel, new-project ON toggle, and save/load. */
#include "b1_performance.h"
#include <assert.h>
#include <stdio.h>
static unsigned active(const struct b1_engine *e,unsigned owner){
 unsigned n=0;
 for(unsigned i=0;i<B1_VOICES;i++)if(e->voice[i].active&&!e->voice[i].releasing&&e->voice[i].owner==owner)n++;
 return n;
}
int main(void){
 static struct b1_engine e;
 static struct b1_performance p;
 static int16_t pcm[256];
 b1_reset(&e);b1_performance_init(&p);
 assert(b1_set_chord_enabled(&p,&e,1));
 assert(b1_assign_chord_key(&p,21,B1_MIN7));
 assert(b1_set_chord_attack(&p,0));
 b1_performance_keys(&p,&e,(1u<<21)|(1u<<4));
 assert(p.held[4].count==4 && !p.pending_mask && active(&e,5)==4);
 b1_performance_keys(&p,&e,0);b1_panic(&e);
 assert(b1_set_chord_attack(&p,100));
 unsigned permutations=0;uint32_t last_first=0;
 for(unsigned t=0;t<32;t++){
  b1_performance_keys(&p,&e,(1u<<21)|(1u<<4));
  struct b1_held *h=&p.held[4];
  assert(h->count==4 && active(&e,5)==1);
  unsigned due_count=0;uint32_t latest=0,first_id=0;
  for(unsigned j=0;j<h->count;j++){
   if(!h->pending[j]){first_id=j;continue;}
   due_count++;
   uint32_t off=h->due[j]-p.clock;
   if(off>latest)latest=off;
  }
  assert(due_count==3 && latest>=1000 && latest<=2866);
  if(t && first_id!=last_first)permutations++;
  last_first=first_id;
  /* This path is the same sound generator used by an active CHORD editor. */
  unsigned ticks=0;
  while(p.pending_mask && ticks++<30)b1_performance_render(&p,&e,pcm,128);
  assert(!p.pending_mask && active(&e,5)==4);
  b1_performance_keys(&p,&e,0);b1_panic(&e);
 }
 assert(permutations>=10);
 assert(b1_set_chord_attack(&p,25));
 b1_performance_keys(&p,&e,(1u<<21)|(1u<<4));
 b1_performance_keys(&p,&e,0);
 assert(!p.pending_mask && active(&e,5)==0);
 struct b1_project pr;
 b1_project_capture(&pr,&p,&e);
 assert(b1_project_valid(&pr));
 b1_performance_init(&p);assert(b1_project_apply(&pr,&p,&e));
 assert(b1_chord_attack(&p)==25 && b1_chord_enabled(&p));
 puts("V2.8 live CHORD ATTACK: 0 simultaneous, 100 staggered, random first voice, sample scheduling, note-off cancel and persistence PASS");
 return 0;
}
