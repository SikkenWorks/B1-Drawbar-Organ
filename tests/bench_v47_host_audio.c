/* Host-only load proxy; strictly NOT FM1 CPU percentage. */
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "b1_engine.h"
static struct b1_engine e;
static int16_t pcm[256];
int main(void){
 b1_reset(&e);
 e.cpu_relief=1u;e.click=70u;e.leakage=25u;e.percussion_enabled=1u;
 e.rotor_enabled=1u;e.rotor_mix=50u;e.sound_mode=B1_SOUND_ROCK;
 for(unsigned b=0;b<3;b++)for(unsigned h=0;h<9;h++)b1_drawbar(&e,b,h,(h<6)?8u:0u);
 clock_t t0=clock();
 volatile int32_t check=0;
 for(unsigned k=0;k<5000;k++){
  if(k%3==0){for(unsigned v=0;v<3;v++)b1_note_on(&e,(uint8_t)(45+4*v),(uint8_t)(v%3),100u);}
  else if(k%3==2){for(unsigned v=0;v<3;v++)b1_note_off(&e,(uint8_t)(45+4*v),(uint8_t)(v%3));}
  b1_render_stereo(&e,pcm,128u);check+=pcm[0];
 }
 printf("%.6f seconds %.3f ns/sample %d\n",(double)(clock()-t0)/CLOCKS_PER_SEC,(double)(clock()-t0)*1e9/CLOCKS_PER_SEC/640000.,(int)check);
}
