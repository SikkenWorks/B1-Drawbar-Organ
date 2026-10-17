#include "../firmware/b1_performance.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
static int active_note(const struct b1_engine *e){
 for(unsigned i=0;i<B1_VOICES;i++)if(e->voice[i].active&&!e->voice[i].releasing)return e->voice[i].note;
 return -1;
}
int main(void){
 struct b1_engine e;struct b1_performance p;b1_reset(&e);b1_performance_init(&p);
 assert(p.global_octave==4);
 b1_performance_keys(&p,&e,1u<<0);
 assert(active_note(&e)==B1_KEYBOARD_LOW_MIDI);
 assert(b1_shift_octave(&p,&e,1,1u<<0) && p.global_octave==5);
 assert(active_note(&e)==B1_KEYBOARD_LOW_MIDI+12);
 assert(b1_shift_octave(&p,&e,-1,1u<<0) && p.global_octave==4);
 assert(active_note(&e)==B1_KEYBOARD_LOW_MIDI);
 /* CHORD assignments require an explicitly activated selector group. */
 p.range[0].hi=19;p.range[2].type=B1_CHORD;
 uint8_t initial=p.chord_map[21];
 assert(b1_assign_chord_key(&p,21,B1_MIN7)==1u && p.chord_map[21]==B1_MIN7);
 assert(b1_assign_chord_key(&p,2,B1_DIM)==0u && p.chord_map[2]==0u);
 assert(b1_assign_chord_key(&p,21,255u)==0u);
 struct b1_project pr;b1_project_capture(&pr,&p,&e);assert(b1_project_valid(&pr));
 struct b1_engine e2;struct b1_performance p2;b1_reset(&e2);b1_performance_init(&p2);
 assert(b1_project_apply(&pr,&p2,&e2)==1 && p2.chord_map[21]==B1_MIN7);
 assert(initial!=B1_MIN7);
 b1_panic(&e2);e2.cpu_relief=1;
 b1_note_on(&e2,60,0,115);
 int16_t out[2*128];int peak=0;
 for(int j=0;j<8;j++){
  b1_render_stereo(&e2,out,128);
  for(unsigned i=0;i<256;i++){int a=out[i]<0?-(int)out[i]:out[i];if(a>peak)peak=a;}
 }
 assert(peak>150);printf("PASS: physical CHORD ASSIGN, project-save, OCT +/- held-key retrigger and nonzero DSP safe audio (peak %d)\n",peak);
}
