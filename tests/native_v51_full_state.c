/* Exact native C regression across dense, transient, harmonic and FX cases.
 * Compile against V4.9 reference and against new V5.1 engine; compare stdout.
 * This is a host check, not FM1 CPU-cycle verification. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "b1_engine.h"
static struct b1_engine e;
static int16_t pcm[512];
static uint64_t digest(uint64_t h,const void *data,unsigned len){
 const unsigned char *p=(const unsigned char *)data;
 for(unsigned i=0;i<len;i++){h^=p[i];h*=UINT64_C(1099511628211);}
 return h;
}
int main(void){
 for(unsigned n=1;n<=12;n+=n<3?1:3)for(unsigned safe=0;safe<2;safe++)
 for(unsigned saw=0;saw<2;saw++)for(unsigned leak=0;leak<2;leak++)
 for(unsigned fx=0;fx<2;fx++){
  memset(&e,0,sizeof(e));b1_reset(&e);
  e.cpu_relief=(uint8_t)safe;
  e.sound_mode=(uint8_t)((n+fx)%2);e.mode_mix_q15=e.sound_mode?32767u:0u;
  e.osc_mode=(uint8_t)saw;e.leakage=(uint8_t)(leak?37:0);
  e.percussion_enabled=(uint8_t)(fx!=0);e.click=(uint8_t)(fx?75:0);
  e.cv_mode=(uint8_t)(fx?4:0);e.cv_depth=(uint8_t)(fx?80:0);
  e.rotor_enabled=(uint8_t)fx;e.rotor_mix=(uint8_t)(fx?65:0);
  e.delay_mix=(uint8_t)(fx?26:0);e.reverb_type=(uint8_t)(fx?2:0);e.reverb_mix=(uint8_t)(fx?30:0);
  for(unsigned bank=0;bank<3;bank++)for(unsigned h=0;h<9;h++)
   b1_drawbar(&e,(uint8_t)bank,(uint8_t)h,(uint8_t)((fx && h%3==0)?5:8));
  for(unsigned v=0;v<n;v++)
   b1_note_on_tagged(&e,(uint8_t)(45+v*3),(uint8_t)(v%3),100u,v+1u);
  uint64_t audio=UINT64_C(14695981039346656037);
  for(unsigned k=0;k<24;k++){
   if(k==8 && (n&1u))b1_note_off(&e,45,0);
   if(k==13 && n>4)b1_note_off(&e,48,1);
   if(k==15 && fx) b1_drawbar(&e,2,3,2);
   b1_render_stereo(&e,pcm,256);
   audio=digest(audio,pcm,sizeof(pcm));
  }
  uint64_t state=digest(UINT64_C(14695981039346656037),&e,sizeof(e));
  printf("n=%u safe=%u saw=%u leak=%u fx=%u pcm=%016llx state=%016llx\n",
   n,safe,saw,leak,fx,(unsigned long long)audio,(unsigned long long)state);
 }
 return 0;
}
