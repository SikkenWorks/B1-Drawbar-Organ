/* B1 V0.3 dedicated DSP and project tests. */
#include "../firmware/b1_performance.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct b1_engine a,b;
static struct b1_performance perf;
static int16_t pcm_a[256*2],pcm_b[256*2];
static void keys(struct b1_engine *e){
 for(unsigned n=0;n<10;n++)b1_note_on(e,(uint8_t)(50+n),n%3u,110);
}
static unsigned long diff(const int16_t *x,const int16_t *y,unsigned n){
 unsigned long total=0;for(unsigned i=0;i<n;i++)total+=(unsigned long)(x[i]>y[i]?x[i]-y[i]:y[i]-x[i]);return total;
}
int main(void){
 b1_reset(&a);b1_reset(&b);b1_performance_init(&perf);
 keys(&a);keys(&b);
 /* Compare dry C/V with independently engaged V3. */
 a.cv_mode=0;b.cv_mode=3;b.cv_depth=90;
 b1_render_stereo(&a,pcm_a,256);b1_render_stereo(&b,pcm_b,256);
 assert(diff(pcm_a,pcm_b,512)>0);
 /* Verify actual rotor has channel movement after pipeline priming. */
 b1_reset(&a);b1_reset(&b);keys(&a);keys(&b);
 a.rotor_enabled=0;b.rotor_enabled=1;b.rotor_fast=1;b.rotor_mix=100;
 b1_render_stereo(&a,pcm_a,256);b1_render_stereo(&b,pcm_b,256);
 assert(diff(pcm_a,pcm_b,512)>0);
 unsigned side=0;for(unsigned i=0;i<256;i++)side+=(pcm_b[i*2]!=pcm_b[i*2+1]);
 assert(side>0);
 b1_reset(&a);b1_performance_init(&perf);
 a.cv_mode=6;a.cv_depth=90;a.rotor_width=71;a.rotor_mix=33;
 struct b1_project project;b1_project_capture(&project,&perf,&a);
 assert(project.version==6);
 b1_reset(&b);assert(b1_project_apply(&project,&perf,&b));
 assert(b.cv_mode==6&&b.cv_depth==90&&b.rotor_width==71&&b.rotor_mix==33);
 project.cv_mode=100;assert(!b1_project_apply(&project,&perf,&b));
 /* Silence with no notes and with drained FX buffers after a reset. */
 b1_reset(&a);b1_render_stereo(&a,pcm_a,256);
 for(unsigned i=0;i<512;i++)assert(pcm_a[i]==0);
 /* Saturation, rotor and C/V for many blocks: fixed-point safety. */
 b1_reset(&a);keys(&a);a.drive=100;a.cv_mode=6;a.cv_depth=100;a.rotor_mix=100;a.rotor_width=100;
 int peak=0;
 for(unsigned t=0;t<200;t++){
  b1_render_stereo(&a,pcm_a,256);
  for(unsigned i=0;i<512;i++){int v=pcm_a[i];int m=v<0?-v:v;if(m>peak)peak=m;}
 }
 assert(peak>0&&peak<=32767);
 printf("B1 V0.3 FX, multitimbral PCM, rotor stereo, silent reset, project-v4, stress: PASS (peak %d)\n",peak);
 return 0;
}
