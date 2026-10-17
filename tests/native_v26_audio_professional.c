/* SPDX-License-Identifier: GPL-3.0-only
 * B1 V2.6 proof: steady 12-note mix vs reference per-voice signal,
 * anti-squaring, percussion factory default, no stuck sound, FX retained.
 */
#include "b1_engine.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
static struct b1_engine fast,ref;
static int16_t a[256],b[256];
static int abs16(int32_t x){return x<0?(int)(-x):(int)x;}
static void run(int mode,int safe,int effects){
 b1_reset(&fast);b1_reset(&ref);
 assert(fast.percussion_enabled==1u);
 fast.sound_mode=ref.sound_mode=(uint8_t)mode;
 fast.cpu_relief=ref.cpu_relief=(uint8_t)safe;
 ref.dense_mix_test_bypass=1u;
 fast.drive=ref.drive=mode?72:18;
 fast.amp_gain=ref.amp_gain=mode?82:40;
 if(effects){
  fast.cv_mode=ref.cv_mode=5u;fast.cv_depth=ref.cv_depth=70u;
  fast.delay_mix=ref.delay_mix=50u;
  fast.reverb_type=ref.reverb_type=4u;fast.reverb_mix=ref.reverb_mix=40u;
 }
 for(int bank=0;bank<3;bank++)for(int h=0;h<9;h++){
  b1_drawbar(&fast,(uint8_t)bank,(uint8_t)h,8u);
  b1_drawbar(&ref,(uint8_t)bank,(uint8_t)h,8u);
 }
 for(int k=0;k<12;k++){
  b1_note_on(&fast,(uint8_t)(48+k),(uint8_t)(k>=8?2:0),115u);
  b1_note_on(&ref,(uint8_t)(48+k),(uint8_t)(k>=8?2:0),115u);
 }
 uint64_t error=0,reference=0;int maxd=0,peak=0,hard=0;
 for(int chunk=0;chunk<90;chunk++){
  b1_render_stereo(&fast,a,128u);
  b1_render_stereo(&ref,b,128u);
  for(int i=0;i<256;i++){
   int d=abs16((int)a[i]-(int)b[i]);if(d>maxd)maxd=d;
   error+=(uint64_t)d;reference+=(uint64_t)abs16(b[i]);
   int ap=abs16(a[i]);if(ap>peak)peak=ap;
   if(ap>26000)hard++;
  }
 }
 assert(peak>500 && peak<26000 && hard==0);
 /* Mathematical coalescing changes only per-voice integer rounding, not
  * tuning, envelopes, effects or loudness. For a 12-note chord, allow
  * at most 1% mean deviation relative to absolute source energy. */
 assert(error*100u<reference && "dense mixer changes sound beyond 1%");
 printf("B3/ROCK=%d SAFE=%d FX=%d crest_peak=%d mean_difference=%.5f%% max_delta=%d\n",
  mode,safe,effects,peak,100.0*(double)error/(double)reference,maxd);
 for(int k=0;k<12;k++){
  b1_note_off(&fast,(uint8_t)(48+k),(uint8_t)(k>=8?2:0));
  b1_note_off(&ref,(uint8_t)(48+k),(uint8_t)(k>=8?2:0));
 }
 for(int chunk=0;chunk<2600;chunk++)b1_render_stereo(&fast,a,128u);
 b1_render_stereo(&fast,a,128u);
 for(int i=0;i<256;i++)assert(a[i]==0);
}
int main(void){for(int mode=0;mode<2;mode++)for(int safe=0;safe<2;safe++)for(int fx=0;fx<2;fx++)run(mode,safe,fx);puts("B1 V2.6 steady-chord signal, full FX, factory percussion and zero-idle PASS");return 0;}
