/* GPL-3.0-only. Deterministic all-effects FM1-host PCM comparison V3.3 vs V3.4. */
#include "b1_engine.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static struct b1_engine e;
static int16_t pcm[256];
int main(int argc,char **argv){
 if(argc!=5)return 2;
 int osc=atoi(argv[1]),sound=atoi(argv[2]);
 if((unsigned)osc>1||(unsigned)sound>1)return 2;
 FILE *f=fopen(argv[4],"wb");if(!f)return 3;
 b1_reset(&e);
 e.cpu_relief=(uint8_t)(atoi(argv[3])?1u:0u); e.osc_mode=(uint8_t)osc;e.sound_mode=(uint8_t)sound;
 e.mode_mix_q15=(uint16_t)(sound?32767:0);
 e.cv_mode=5;e.cv_depth=78;e.rotor_enabled=1;e.rotor_fast=1;
 e.rotor_mix=87;e.rotor_width=78;e.drive=30;
 e.leakage=35;e.click=0;e.delay_mix=30;e.delay_time=55;
 e.reverb_type=4;e.reverb_mix=38;e.reverb_decay=68;
 for(int bank=0;bank<3;bank++)for(int h=0;h<9;h++)b1_drawbar(&e,bank,h,(uint8_t)(h%3==0?8:5));
 for(int t=0;t<1150;t++){
  if(t==0)for(int v=0;v<12;v++)b1_note_on_tagged(&e,(uint8_t)(48+v),(uint8_t)(v%3),110,(uint32_t)(v+1));
  if(t==220)e.osc_mode^=1u;
  if(t==430)e.osc_mode^=1u;
  if(t==620)for(uint32_t owner=1;owner<=12;owner++)b1_note_off_tagged(&e,owner);
  b1_render_stereo(&e,pcm,128);
  if(fwrite(pcm,sizeof(pcm),1,f)!=1)return 4;
 }
 fclose(f);return 0;
}
