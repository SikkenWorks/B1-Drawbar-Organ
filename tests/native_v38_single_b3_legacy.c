/* SPDX-License-Identifier: GPL-3.0-only
 * Playback fingerprint: single-key B3/SAFE percussion must retain V3.7 PCM.
 * 3 manuals, drawbars and unused effects stay accessible; no project ABI change.
 */
#include "b1_engine.h"
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
static struct b1_engine e;
static int16_t pcm[256];
int main(void){
 b1_reset(&e);e.cpu_relief=1;e.sound_mode=B1_SOUND_B3;e.mode_mix_q15=0u;
 e.click=0u;e.leakage=13u;e.cv_mode=3u;e.cv_depth=44u;
 e.rotor_enabled=1;e.rotor_mix=60u;
 e.delay_mix=20u;e.reverb_type=2u;e.reverb_mix=20u;
 b1_note_on_tagged(&e,60u,0u,100u,1u);
 uint64_t hash=UINT64_C(1469598103934665603);
 for(unsigned frame=0;frame<128;frame++){
  if(frame==64) b1_note_off_tagged(&e,1u);
  b1_render_stereo(&e,pcm,128u);
  for(unsigned i=0;i<256;i++){
   hash^=(uint16_t)pcm[i];hash*=UINT64_C(1099511628211);
  }
 }
 assert(hash==UINT64_C(0x87a297fbf56dda4d));
 printf("PASS: V3.7 -> V3.8 bit-exact single-key B3/SAFE PCM hash=%016llx\n",(unsigned long long)hash);
 return 0;
}
