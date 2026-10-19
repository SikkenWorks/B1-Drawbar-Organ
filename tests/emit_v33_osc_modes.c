/* SPDX-License-Identifier: GPL-3.0-only. Host PCM reference only, no FM1 hardware capture. */
#include "b1_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
static struct b1_engine e;
static int16_t pcm[128u*2u];
int main(int argc,char **argv){
 if(argc!=3)return 2;
 int m=atoi(argv[1]);if(m<0||m>1)return 2;
 FILE *f=fopen(argv[2],"wb");if(!f)return 3;
 b1_reset(&e);e.cpu_relief=1u;e.percussion_enabled=0u;
 e.rotor_enabled=0u;e.rotor_mix=0u;e.sound_mode=B1_SOUND_B3;
 e.cv_mode=0u;e.drive=0u;e.leakage=0u;
 /* Bright clear organ registration, no clipping. */
 for(int j=0;j<9;j++)b1_drawbar(&e,0,(uint8_t)j,j<5?7u:0u);
 e.osc_mode=(uint8_t)m;
 b1_note_on(&e,57u,0u,120u);
 for(int i=0;i<690;i++){
  b1_render_stereo(&e,pcm,128u);
  if(fwrite(pcm,sizeof(pcm),1,f)!=1){fclose(f);return 4;}
 }
 fclose(f);return 0;
}
