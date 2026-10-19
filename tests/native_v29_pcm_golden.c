/* Bit-exact V2.8 -> V2.9 golden PCM comparison, not a physical FM1 test. */
#include "b1_engine.h"
#include <stdio.h>
#include <stdint.h>
static struct b1_engine e;
static int16_t pcm[256];
static uint64_t h;
static void hash_pcm(void){
 for(unsigned i=0;i<256;i++){h^=(uint16_t)pcm[i];h*=1099511628211ULL;}
}
int main(void){
 for(int mode=0;mode<2;mode++)for(int safe=0;safe<2;safe++){
  b1_reset(&e);e.sound_mode=(uint8_t)mode;e.mode_mix_q15=mode?32767u:0u;e.cpu_relief=(uint8_t)safe;
  e.cv_mode=5;e.cv_depth=79;e.rotor_enabled=1;e.rotor_mix=65;
  e.delay_mix=47;e.delay_feedback=55;e.delay_time=75;e.delay_spread=62;
  e.reverb_type=4;e.reverb_mix=60;e.reverb_decay=80;e.reverb_damp=73;
  e.click=0;e.leakage=15;
  for(int b=0;b<3;b++)for(int d=0;d<9;d++)b1_drawbar(&e,(uint8_t)b,(uint8_t)d,8);
  for(int j=0;j<12;j++)b1_note_on_tagged(&e,(uint8_t)(52+j),(uint8_t)(j%3),100,(uint32_t)(j+1));
  h=1469598103934665603ULL;
  for(int t=0;t<2400;t++){
   if(t==1000)for(int j=0;j<12;j++)b1_note_off_tagged(&e,(uint32_t)(j+1));
   if(t==1800)for(int j=0;j<6;j++)b1_note_on_tagged(&e,(uint8_t)(60+j),0,108,(uint32_t)(j+101));
   b1_render_stereo(&e,pcm,128);hash_pcm();
  }
  printf("mode=%d safe=%d golden=%016llx\n",mode,safe,(unsigned long long)h);
 }
 return 0;
}
