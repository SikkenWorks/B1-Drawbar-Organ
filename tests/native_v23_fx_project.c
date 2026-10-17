/* SPDX-License-Identifier: GPL-3.0-only. V2.3 FX and complete project persistence. */
#include "../firmware/b1_engine.h"
#include "../firmware/b1_performance.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static struct b1_engine e;
static struct b1_performance p;
static int16_t pcm[256];
static unsigned long long render(unsigned mode,unsigned type,int cpu_safe){
 b1_reset(&e);e.cpu_relief=(uint8_t)cpu_safe;
 e.rotor_enabled=0;e.click=0;e.leakage=0;
 e.cv_mode=(uint8_t)mode;e.cv_depth=92;
 if(type==1){e.delay_mix=68;e.delay_time=18;e.delay_feedback=43;e.delay_spread=77;}
 else if(type>=2){e.reverb_type=(uint8_t)(type-1);e.reverb_mix=85;e.reverb_decay=70;e.reverb_damp=48;}
 b1_note_on(&e,60,0,100);b1_note_on(&e,64,0,100);
 unsigned long long sum=0;
 for(int i=0;i<96;i++){
  b1_render_stereo(&e,pcm,128);
  if(i>=16)for(int n=0;n<256;n++){int x=pcm[n];sum+=(unsigned)(x<0?-x:x);}
 }
 assert(sum>100000u);return sum;
}
int main(void){
 for(int safe=0;safe<=1;safe++){
  unsigned long long dry=render(0,0,safe);
  unsigned long long v1=render(1,0,safe),v3=render(3,0,safe),c2=render(5,0,safe);
  unsigned long long delay=render(0,1,safe);
  unsigned long long spring=render(0,2,safe),room=render(0,3,safe);
  unsigned long long hall=render(0,4,safe),cat=render(0,5,safe);
  printf("DSP=%d sums dry=%llu V1=%llu V3=%llu C2=%llu delay=%llu spring=%llu room=%llu hall=%llu cat=%llu\n",safe,dry,v1,v3,c2,delay,spring,room,hall,cat);
  assert(dry!=v1 && v1!=v3 && v3!=c2 && delay!=dry);
  assert(spring!=dry&&room!=dry&&hall!=dry&&cat!=dry);
  assert(spring!=room && room!=hall && hall!=cat);
 }
 b1_reset(&e);b1_performance_init(&p);
 p.global_octave=6;p.selected_bank=2;p.scale_type=17;p.scale_root=9;p.keylock=1;p.guide=0;
 e.cv_mode=6;e.cv_depth=73;e.delay_mix=82;e.delay_time=71;e.delay_feedback=60;e.delay_spread=33;
 e.reverb_type=4;e.reverb_mix=52;e.reverb_decay=91;e.reverb_damp=18;
 e.rotor_slow_pct=67;e.rotor_fast_pct=136;e.rotor_drum_pct=91;
 e.percussion_enabled=1;e.percussion_harmonic=2;e.sound_mode=1;e.amp_gain=82;e.amp_tone=43;
 b1_drawbar(&e,2,8,7);
 struct b1_project pr; b1_project_capture(&pr,&p,&e);
 assert(pr.version==6u && b1_project_valid(&pr));
 b1_reset(&e);b1_performance_init(&p);
 assert(b1_project_apply(&pr,&p,&e));
 assert(p.global_octave==6&&p.selected_bank==2&&p.scale_type==17&&p.scale_root==9&&p.keylock==1);
 assert(e.cv_mode==6&&e.cv_depth==73&&e.delay_mix==82&&e.delay_time==71&&e.delay_feedback==60&&e.delay_spread==33);
 assert(e.reverb_type==4&&e.reverb_mix==52&&e.reverb_decay==91&&e.reverb_damp==18);
 assert(e.rotor_slow_pct==67&&e.rotor_fast_pct==136&&e.rotor_drum_pct==91&&e.drawbars[2][8]==7);
 assert(e.percussion_enabled==1&&e.percussion_harmonic==2&&e.sound_mode==1&&e.amp_gain==82&&e.amp_tone==43);
 puts("B1 V2.3: 4 reverb types, delay, CV, full/safe DSP and all musical settings persistence PASS");
 return 0;
}
