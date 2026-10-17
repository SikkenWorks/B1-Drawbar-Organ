/* SikkenWorks B1 V2.0. Regression: click response, real note-off silence,
 * 91-wheel phase continuity and archived click settings. */
#include "b1_performance.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#define NS 1024u
static int16_t pcm[NS*2], ref[NS*2];
static uint32_t sum_abs_diff(const int16_t *a,const int16_t *b){
 uint32_t total=0;for(uint32_t i=0;i<NS*2;i++){
  int32_t d=(int32_t)a[i]-(int32_t)b[i];total+=(uint32_t)(d<0?-d:d);
 }return total;
}
static int is_zero(const int16_t *a,uint32_t len){for(uint32_t i=0;i<len;i++)if(a[i]!=0)return 0;return 1;}
static void click_take(uint8_t level,int16_t *dest,uint8_t safe){
 static struct b1_engine e;
 b1_reset(&e);e.cpu_relief=safe;e.click=level;e.percussion_enabled=0;e.leakage=0;e.drive=0;e.cv_mode=0;
 b1_note_on_tagged(&e,60u,0u,100u,1u);
 b1_render_stereo(&e,dest,NS);
}
int main(void){
 static struct b1_engine e;
 assert(sizeof(struct b1_engine)<160000u);
 b1_reset(&e);assert(e.click==0u);
 uint8_t levels[]={0,1,10,25,50,75,100};
 click_take(0,ref,1u);
 uint32_t previous=0;
 for(unsigned i=1;i<sizeof(levels);i++){
  click_take(levels[i],pcm,1u);
  uint32_t difference=sum_abs_diff(pcm,ref);
  printf("CLICK %3u, |PCM - CLICK 0| = %u\n",levels[i],difference);
  assert(difference>0u);
  if(i>1u)assert(difference>previous);
  previous=difference;
 }
 /* Both the regular FX chain and SAFE DSP must suppress residual wheel
  * signals after the last physical key is released. The effects don't keep
  * mixing unkeyed tonewheels after the output fade reaches exactly zero. */
 for(unsigned mode=0;mode<2;mode++){
  b1_reset(&e);e.cpu_relief=(uint8_t)mode;
  e.click=0u;e.percussion_enabled=0u;
  uint8_t wheel=b1_tonewheel_index(60,2);
  uint32_t initial=e.wheel_phase[wheel],inc=e.wheel_inc[wheel],frame=e.wheel_frame;
  b1_note_on_tagged(&e,60,0,100,2);b1_render_stereo(&e,pcm,NS);
  assert(!is_zero(pcm,NS*2u));
  b1_note_off_tagged(&e,2);
  for(unsigned i=0;i<20;i++)b1_render_stereo(&e,pcm,NS);
  assert(is_zero(pcm,NS*2u));
  assert(e.output_gate_q15==0u&&e.fx_cleared==1u);
  assert(e.cabinet_hp==0&&e.amp_hp==0&&e.rotor_lp1==0&&e.rotor_lp2==0);
  for(unsigned i=0;i<B1_CV_SAMPLES;i++)assert(e.cv_line[i]==0);
  for(unsigned i=0;i<B1_ROTARY_SAMPLES;i++)assert(e.rotor_line[i]==0&&e.drum_line[i]==0);
  assert(e.wheel_frame>frame+NS);
  /* The next note samples phase computed from the original, free-running
   * wheel clock and must still produce sound. */
  b1_note_on_tagged(&e,60,0,100,3);b1_render_stereo(&e,pcm,NS);
  assert(!is_zero(pcm,NS*2u));
  uint32_t elapsed=e.wheel_phase_stamp[wheel];
  assert(e.wheel_phase[wheel]==initial+inc*elapsed);
  b1_panic(&e);b1_render_stereo(&e,pcm,NS);assert(is_zero(pcm,NS*2u));
  printf("PASS no-keys complete silence in %s mode\n",mode?"SAFE":"FULL");
 }
 /* A new project defaults to CLICK 0, while user-saved nonzero settings
  * still round-trip without breaking V5 project compatibility. */
 static struct b1_performance perf;static struct b1_project patch;
 b1_reset(&e);b1_performance_init(&perf);assert(e.click==0u);
 e.click=71;b1_project_capture(&patch,&perf,&e);assert(b1_project_valid(&patch));
 b1_reset(&e);assert(e.click==0u);
 assert(b1_project_apply(&patch,&perf,&e));assert(e.click==71u);
 puts("PASS Key Click 0..100, preset compatibility, no unkeyed tonewheel audio");
 return 0;
}
