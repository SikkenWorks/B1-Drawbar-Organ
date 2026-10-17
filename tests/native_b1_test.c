#include "../firmware/b1_performance.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
static uint8_t voice_exists(const struct b1_engine *e,uint8_t note,uint32_t owner){
 for(unsigned i=0;i<B1_VOICES;i++)if(e->voice[i].active&&e->voice[i].note==note&&e->voice[i].owner==owner)return 1;
 return 0;
}
int main(void){
 struct b1_engine e;struct b1_performance p;
 b1_reset(&e);b1_performance_init(&p);
 /* Regression for explicitly configured CHORD split (no longer the default). */
 p.range[0].hi=9;p.range[1]=(struct b1_range){10,19,B1_ORGAN,1,-1,0};
 p.range[2].type=B1_CHORD;
 assert(p.global_octave==4);
 assert(e.drawbars[0][0]==8 && e.drawbars[2][5]==8);
 assert(p.range[2].type==B1_CHORD);
 b1_performance_keys(&p,&e,(1u<<21));assert(b1_active_chord(&p)==B1_MIN);
 b1_performance_keys(&p,&e,(1u<<21)|(1u<<1));
 assert(p.held[1].count==3);
 assert(voice_exists(&e,54,2));assert(voice_exists(&e,57,2));assert(voice_exists(&e,61,2));
 b1_performance_keys(&p,&e,1u<<1);
 assert(p.held[1].count==3); /* release modifier does not cut existing chord */
 b1_performance_keys(&p,&e,0u);
 assert(p.held[1].count==0);assert(b1_active_chord(&p)==255);
 b1_performance_keys(&p,&e,(1u<<20)|(1u<<22));assert(b1_active_chord(&p)==B1_MAJ7);
 b1_performance_keys(&p,&e,1u<<20);assert(b1_active_chord(&p)==B1_MAJ);
 b1_release_all(&p,&e);b1_panic(&e);
 p.global_octave=5;
 b1_performance_keys(&p,&e,1u<<20);assert(b1_active_chord(&p)==B1_MAJ);
 b1_performance_keys(&p,&e,(1u<<20)|(1u<<0));assert(voice_exists(&e,65,1));
 b1_release_all(&p,&e);b1_panic(&e);
 p.scale_type=1;p.scale_root=0;p.keylock=1;
 assert(b1_note_allowed(&p,60));assert(!b1_note_allowed(&p,61));
 assert(b1_quantize_note(&p,61)==60);
 struct b1_project pr;b1_project_capture(&pr,&p,&e);
 b1_reset(&e);b1_performance_init(&p);assert(b1_project_apply(&pr,&p,&e));
 assert(p.keylock==1&&p.global_octave==5); /* Octave must persist in V6. */
 struct b1_project invalid=pr;invalid.chords[0]=255;assert(!b1_project_apply(&invalid,&p,&e));
 invalid=pr;invalid.checksum ^= 1;assert(!b1_project_apply(&invalid,&p,&e));
 int16_t pcm[256];b1_performance_keys(&p,&e,1u<<0);b1_render_stereo(&e,pcm,128);
 int peak=0;for(int i=0;i<256;i++){int n=pcm[i]<0?-pcm[i]:pcm[i];if(n>peak)peak=n;}assert(peak>0&&peak<32768);
 printf("B1 V0.8: routing, chords, octave-independent modifiers, notes, project integrity and PCM PASSED (peak=%d)\n",peak);
 return 0;
}
