/* Independently calculate V5.7's DAC stream and control state. */
#include "b1_output.h"
#include <assert.h>
#include <stdio.h>
static uint32_t random_state=1;
static uint32_t next(void){return random_state=random_state*1664525u+1013904223u;}
int main(void){
 int16_t pcm[512];
 int32_t output[512];
 for(unsigned block=0;block<4096;block++){
  for(unsigned i=0;i<512;i++)pcm[i]=(int16_t)(next()>>16);
  pcm[0]=INT16_MIN;pcm[1]=INT16_MAX;
  uint16_t target=(block%3==0)?32767:(block%3==1)?0:(next()>>17);
  uint16_t gain=(block&4)?target:next()>>17;
  uint16_t fade=(block&8)?4096:next()%4097;
  uint16_t expected_gain=gain, expected_fade=fade;
  uint8_t ready=!!(block&1), resume=!!(block&2);
  int32_t last[2]={(int16_t)(next()>>16)*128,(int16_t)(next()>>16)*128};
  b1_pack_output(pcm,output,256,target,&gain,&fade,ready,resume,last[0],last[1]);
  for(unsigned t=0;t<256;t++){
   int delta=target-expected_gain;
   if(ready)expected_gain+=delta<0?-((-delta+255)/256):(delta+255)/256;
   for(unsigned ch=0;ch<2;ch++){
    int32_t v=(int32_t)pcm[t*2+ch]*expected_gain/32767;
    if(expected_fade<4096)v=v*expected_fade/4096;
    if(resume && t<64)v=((last[ch]/128)*(int32_t)(64-t)+v*(int32_t)t)/64;
    assert(output[t*2+ch]==(ready?v*128:0));
   }
   if(ready && expected_fade<4096)expected_fade++;
  }
  assert(fade==expected_fade && gain==expected_gain);
 }
 puts("PASS: 2097152 DAC words and 4096 final gain/fade states vs V5.7 arithmetic");
}
