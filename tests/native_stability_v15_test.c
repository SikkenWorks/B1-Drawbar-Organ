/* B1 V1.5: deterministic stress, canary output, voice accounting,
 * random CHORD/range edits, project corruption and modulo wrap tests.
 * Run under ASan + UBSan on the host in addition to the FM1 crossbuild. */
#include "b1_performance.h"
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
static struct b1_engine e, ecopy;
static struct b1_performance p;
static uint32_t random_state=0x1c653472u;
static uint32_t rng(void){uint32_t x=random_state;x^=x<<13;x^=x>>17;x^=x<<5;return random_state=x;}
static struct {uint32_t before;int16_t pcm[128*2];uint32_t after;} guarded;
static uint32_t count_upper(void){uint32_t n=0;for(unsigned i=0;i<B1_VOICES;i++)if(e.voice[i].active&&!e.voice[i].releasing&&e.voice[i].bank==0u)n++;return n;}
static void render(unsigned frames){
 guarded.before=0xdecadabau;guarded.after=0x1a2b3c4du;
 b1_render_stereo(&e,guarded.pcm,frames);
 assert(guarded.before==0xdecadabau&&guarded.after==0x1a2b3c4du);
}
static uint32_t crc(const struct b1_project *pr){uint32_t h=2166136261u;const uint8_t *b=(const uint8_t*)pr;
 for(unsigned i=0;i<sizeof(*pr)-sizeof(pr->checksum);i++){h^=b[i];h*=16777619u;}return h;}
int main(void){
 b1_reset(&e);b1_performance_init(&p);
 /* Trigger every voice, followed by stealing under busy, partially-releasing state. */
 for(unsigned i=0;i<12;i++)b1_note_on_tagged(&e,(uint8_t)(50+i),0,100,i+1);
 assert(e.upper_held==12);
 render(128);b1_note_off_tagged(&e,2);b1_note_off_tagged(&e,4);render(128);
 b1_note_on_tagged(&e,72,0,100,300);assert(e.upper_held==count_upper());
 b1_note_on_tagged(&e,73,0,100,301);assert(e.upper_held==count_upper());
 for(unsigned i=0;i<2000;i++){
  uint32_t r=rng();uint8_t note=(uint8_t)(30+(r%80)), bank=(uint8_t)((r>>8)%3u);
  switch(r%8u){
   case 0: b1_note_on_tagged(&e,note,bank,127u,(r>>11)|1u);break;
   case 1: b1_note_off(&e,note,bank);break;
   case 2: b1_note_off_tagged(&e,(r>>11)|1u);break;
   case 3: e.drive=(uint8_t)(r%101u);e.sound_mode=(uint8_t)((r>>4)&1u);break;
   case 4: e.rotor_fast=(uint8_t)((r>>5)&1u);e.cv_mode=(uint8_t)((r>>7)%7u);break;
   case 5: b1_drawbar(&e,bank,(uint8_t)((r>>7)%9),(uint8_t)((r>>10)%9));break;
   case 6: e.cabinet_mix=(uint8_t)((r>>4)%101);e.amp_gain=(uint8_t)((r>>11)%101);break;
   default: e.percussion_enabled=(uint8_t)((r>>6)&1u);break;
  }
  if(!(i%31))b1_panic(&e);
  if(!(i%7))render(128);else render(16);
  assert(e.upper_held==count_upper());
  for(unsigned j=0;j<B1_VOICES;j++)if(e.voice[j].active){
   assert(e.voice[j].bank<B1_BANKS&&e.voice[j].note<=127u);
   for(unsigned h=0;h<B1_HARMONICS;h++)assert(e.voice[j].wheel[h]<B1_TONEWHEELS);
  }
 }
 b1_panic(&e);assert(e.upper_held==0u);
 /* MIDI/physical chord storm plus key-up cancellation of scheduled notes. */
 b1_reset(&e);b1_performance_init(&p);p.range[0].hi=19;p.range[2].type=B1_CHORD;p.range[2].attack=100;
 for(unsigned i=0;i<500;i++){
  uint32_t flags=(1u<<(20u+(rng()%7u)))|(1u<<(rng()%20u));
  b1_performance_keys(&p,&e,flags);
  b1_performance_render(&p,&e,guarded.pcm,128);
  b1_performance_keys(&p,&e,0u);
  b1_performance_render(&p,&e,guarded.pcm,128);
  assert(e.upper_held==count_upper());
 }
 b1_release_all(&p,&e);b1_panic(&e);
 struct b1_project pr,bad;b1_project_capture(&pr,&p,&e);
 ecopy=e;bad=pr;bad.ranges[1].type=B1_ORGAN;bad.ranges[1].bank=255;bad.checksum=crc(&bad);
 assert(!b1_project_apply(&bad,&p,&e));assert(memcmp(&e,&ecopy,sizeof(e))==0);
 bad=pr;bad.ranges[0].attack=255;bad.checksum=crc(&bad);
 assert(!b1_project_apply(&bad,&p,&e));
 bad=pr;bad.drawbars[2][4]=255;bad.checksum=crc(&bad);
 assert(!b1_project_apply(&bad,&p,&e));
 assert(b1_project_apply(&pr,&p,&e));
 /* An idle gap must preserve pitch phase of both live and dormant wheels. */
 b1_reset(&e);uint8_t w=b1_tonewheel_index(60,2);uint32_t initial=e.wheel_phase[w],step=e.wheel_inc[w];
 render(128);render(128);assert(e.wheel_phase[w]==initial);
 b1_note_on_tagged(&e,60,0,100,77);render(1);
 assert(e.wheel_phase[w]==initial+step*257u);
 puts("B1 V1.5: voice stealing, 2k randomized DSP cases, chord key-up, canaries, project tampering, idle phase: PASS");
 return 0;
}
