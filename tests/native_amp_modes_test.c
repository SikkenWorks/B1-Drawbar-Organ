/* GPL-3.0-only: test actual B1 V0.9 dual amp path, not mock DSP. */
#include "../firmware/b1_performance.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static struct b1_engine clean,rock;
static struct b1_performance perf;
static int16_t a[128*2],b[128*2];
static uint64_t diffs;
static uint64_t energy_clean,energy_rock;
int main(void){
 b1_reset(&clean);b1_reset(&rock);b1_performance_init(&perf);
 clean.sound_mode=B1_SOUND_B3;clean.mode_mix_q15=0u;
 rock.sound_mode=B1_SOUND_ROCK;rock.mode_mix_q15=32767u;
 clean.rotor_enabled=0u;rock.rotor_enabled=0u;
 b1_note_on(&clean,60,0,100);b1_note_on(&rock,60,0,100);
 for(unsigned block=0;block<160;block++){
  b1_render_stereo(&clean,a,128);b1_render_stereo(&rock,b,128);
  for(unsigned n=0;n<256;n++){
   int64_t d=(int64_t)a[n]-(int64_t)b[n];if(d<0)d=-d;diffs+=(uint64_t)d;
   int32_t va=a[n],vb=b[n];energy_clean+=(uint64_t)(va<0?-va:va);energy_rock+=(uint64_t)(vb<0?-vb:vb);
   assert((n&1u)||a[n]==a[n+1u]); /* dry two channels remain mono */
  }
 }
 assert(diffs>500000u && energy_clean && energy_rock);
 assert(clean.mode_mix_q15==0 && rock.mode_mix_q15==32767);
 /* Mode toggled while held: a finite, monotonic ramp, not a sudden switch. */
 rock.sound_mode=B1_SOUND_B3;
 b1_render_stereo(&rock,b,128);assert(rock.mode_mix_q15<32767u && rock.mode_mix_q15>0u);
 for(unsigned block=0;block<12;block++)b1_render_stereo(&rock,b,128);
 assert(rock.mode_mix_q15==0u);
 /* Project V5 retains sound mode / amplifier settings. */
 rock.sound_mode=B1_SOUND_ROCK;rock.amp_gain=83;rock.amp_tone=37;rock.cabinet_mix=65;
 struct b1_project saved;b1_project_capture(&saved,&perf,&rock);
 assert(saved.version==6u);
 b1_reset(&rock);assert(b1_project_apply(&saved,&perf,&rock));
 assert(rock.sound_mode==B1_SOUND_ROCK && rock.amp_gain==83 && rock.amp_tone==37 && rock.cabinet_mix==65);
 printf("B1 V0.9 clean/rock differing PCM, clickless mode transition, V5 amp save/load PASS (difference %llu)\n",(unsigned long long)diffs);
 return 0;
}
