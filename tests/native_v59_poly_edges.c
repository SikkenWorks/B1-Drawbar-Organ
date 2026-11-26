#include "b1_engine.h"
#include <stdio.h>
#include <assert.h>
static struct b1_engine e;
static int16_t pcm[512];
static uint64_t hash(uint64_t h,const void *p,size_t n){
 const uint8_t *b=p;while(n--){h^=*b++;h*=UINT64_C(1099511628211);}return h;
}
int main(void){
 static const unsigned sizes[]={1,15,16,17,31,32,63,65,127,128,129,255,256};
 for(unsigned scene=0;scene<64;scene++){
  b1_reset(&e);e.cpu_relief=scene&1;e.percussion_enabled=(scene>>1)&1;
  e.osc_mode=(scene>>2)&1;e.leakage=(scene&8)?100:0;e.click=(scene&16)?100:0;
  for(unsigned b=0;b<3;b++)for(unsigned h=0;h<9;h++)b1_drawbar(&e,b,h,8);
  for(unsigned i=0;i<12;i++)b1_note_on_tagged(&e,36+i*4,i%3,127,i+1);
  if(scene&32)e.wheel_frame=UINT32_MAX-48;
  uint64_t digest=1;
  for(unsigned k=0;k<100;k++){
   unsigned n=sizes[(k+scene)%13];
   if(k==28||k==41)b1_note_off_tagged(&e,3);
   if(k==30)b1_note_off_tagged(&e,7);
   if(k==32)b1_note_on_tagged(&e,73,1,111,99);
   if(k==36)e.osc_mode^=1;
   if(k==52)b1_drawbar(&e,0,6,2);
   if(k==55)e.percussion_env=2;
   if(k==70)for(unsigned i=0;i<15;i++)b1_note_on_tagged(&e,40+i,i%3,100,101+i);
   if(k==82)for(unsigned i=0;i<15;i++)b1_note_off_tagged(&e,101+i);
   b1_render_stereo(&e,pcm,n);
   digest=hash(digest,pcm,n*2*sizeof(*pcm));digest=hash(digest,&e,sizeof(e));
  }
  printf("%u %016llx\n",scene,(unsigned long long)digest);
 }
}
