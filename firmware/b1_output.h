/* GPL-3.0-only. All controls here have a single main-thread writer
 * and remain stable while the audio interrupt is running. */
#pragma once
#include "b1_master_gain.h"
static inline void b1_pack_output(const int16_t *pcm, int32_t *dst, uint32_t frames,
 uint16_t target, uint16_t *gain, uint16_t *fade_state, uint8_t ready,
 uint8_t resume, int32_t last_l, int32_t last_r){
 if(!ready){
  for(uint32_t i=0;i<frames*2u;i++)dst[i]=0;
  return;
 }
 uint16_t smoothed=*gain, fade=*fade_state;
 if(smoothed==target && fade==4096u && !resume){
  if(target==32767u){
   for(uint32_t i=0;i<frames*2u;i++)dst[i]=(int32_t)pcm[i]*128;
  }else{
   for(uint32_t i=0;i<frames*2u;i++)dst[i]=b1_master_gain_exact(pcm[i],target)*128;
  }
  return;
 }
 for(uint32_t t=0;t<frames;t++){
  int32_t diff=(int32_t)target-smoothed;
  smoothed+=diff<0?-((-diff+255)/256):(diff+255)/256;
  for(uint32_t ch=0;ch<2u;ch++){
   uint32_t i=t*2u+ch;
   int32_t sample=b1_master_gain_exact(pcm[i],smoothed);
   if(fade<4096u)sample=(sample*(int32_t)fade)/4096;
   if(resume && t<64u){
    int32_t previous=(ch?last_r:last_l)/128;
    sample=(previous*(int32_t)(64u-t)+sample*(int32_t)t)/64;
   }
   dst[i]=sample*128;
  }
  if(fade<4096u)++fade;
 }
 *gain=smoothed; *fade_state=fade;
}
