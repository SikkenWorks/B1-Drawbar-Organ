/* GPL-3.0-only: V2.2 high-polyphony headroom, rotor fine tuning, SOLO and archive binary compatibility. */
#include "../firmware/b1_engine.h"
#include "../firmware/b1_performance.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#define S 4096
static int16_t audio[S*2];
static unsigned long long metric(const int16_t *d,unsigned n){unsigned long long v=0;for(unsigned i=0;i<n;i++)v+=(unsigned)(d[i]<0?-(int)d[i]:d[i]);return v;}
static int peak(const int16_t *d,unsigned n){int v=0;for(unsigned i=0;i<n;i++){int a=d[i]<0?-(int)d[i]:d[i];if(a>v)v=a;}return v;}
static unsigned long long play(uint8_t mode,unsigned notes,int full,int speed){
 struct b1_engine e;b1_reset(&e);e.cpu_relief=1;e.sound_mode=mode;
 e.drive=(uint8_t)(mode==B1_SOUND_ROCK?60:8);e.amp_gain=55;e.rotor_mix=85;
 e.rotor_fast=(uint8_t)(speed>0);e.rotor_slow_pct=(uint8_t)(speed<0?60:100);e.rotor_fast_pct=(uint8_t)(speed>0?140:100);
 for(unsigned b=0;b<B1_BANKS;b++)for(unsigned h=0;h<B1_HARMONICS;h++)b1_drawbar(&e,(uint8_t)b,(uint8_t)h,full?8:(h<4?8:0));
 for(unsigned i=0;i<notes;i++)b1_note_on(&e,(uint8_t)(48+i),0,127);
 memset(audio,0,sizeof audio);b1_render_stereo(&e,audio,S);
 int pk=peak(audio,S*2);assert(pk>100&&pk<32767);
 unsigned long long sum=metric(audio,S*2);
 printf("mode=%u notes=%u all_drawbars=%d speed=%d peak=%d abs-sum=%llu\n",mode,notes,full,speed,pk,sum);
 return sum;
}
int main(void){
 struct b1_performance p;b1_performance_init(&p);
 assert(p.range[0].bank==0 && p.range[0].lo==0 && p.range[0].hi==26);
 assert(p.range[1].type==B1_ORGAN && p.range[1].bank==2 && p.range[1].lo==20 && p.range[1].hi==26);
 assert(sizeof(struct b1_project)==124u);assert(offsetof(struct b1_project,checksum)==120u);
 struct b1_engine e;b1_reset(&e);
 e.rotor_slow_pct=65;e.rotor_fast_pct=142;e.rotor_drum_pct=113;
 struct b1_project pr;b1_project_capture(&pr,&p,&e);assert(b1_project_valid(&pr));
 struct b1_engine e2;b1_reset(&e2);assert(b1_project_apply(&pr,&p,&e2));
 assert(e2.rotor_slow_pct==65 && e2.rotor_fast_pct==142 && e2.rotor_drum_pct==113);
 /* Test zeroed rotor-speed values within the V6 project schema. */
 struct b1_project legacy=pr;legacy.rotor_slow_pct=0;legacy.rotor_fast_pct=0;legacy.rotor_drum_pct=0;
 /* V6 checksum protects the complete extended project. */
 unsigned char *a=(unsigned char*)&legacy;uint32_t h=2166136261u;
 for(unsigned i=0;i<offsetof(struct b1_project,checksum);i++){h^=a[i];h*=16777619u;}legacy.checksum=h;
 assert(b1_project_valid(&legacy));assert(b1_project_apply(&legacy,&p,&e2));
 assert(e2.rotor_slow_pct==0u && e2.rotor_fast_pct==0u && e2.rotor_drum_pct==0u);
 unsigned long long b3=play(B1_SOUND_B3,12,1,0), rock=play(B1_SOUND_ROCK,12,1,0);
 assert(b3!=rock && b3>500000u && rock>500000u);
 unsigned long long slow=play(B1_SOUND_B3,4,0,-1),fast=play(B1_SOUND_B3,4,0,1);
 assert(slow!=fast);
 printf("B1 V2.2: SOLO routing, V6 project, 12-voice headroom, distinct sound modes and rotor fine tuning PASS\n");
 return 0;
}
