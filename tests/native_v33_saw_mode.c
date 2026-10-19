/* GPL-3.0-only: B1 V3.3 experimental SAW oscillator, button chord, project persistence. */
#include "../firmware/b1_engine.h"
#include "../firmware/b1_performance.h"
#include "../firmware/b1_mode_keys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
static void check(int v,const char *label){if(!v){fprintf(stderr,"FAIL: %s\n",label);exit(1);}}
static struct b1_engine e,e2;
static struct b1_performance p,p2;
static struct b1_project pr;
static int16_t organ[8192],saw[8192],organ_return[8192];
static uint64_t energy(const int16_t *x,int n){uint64_t v=0;for(int i=0;i<n;i++)v+=(uint64_t)((int32_t)x[i]*x[i]);return v;}
int main(void){
 check(sizeof(struct b1_project)==124u,"same V6 project size");
 check(offsetof(struct b1_project,checksum)==120u,"same V6 checksum offset");
 check(offsetof(struct b1_project,osc_mode)==119u,"reuse old zero padding");
 b1_reset(&e);b1_performance_init(&p);
 check(e.osc_mode==B1_OSC_ORGAN&&e.osc_blend_q15==0u,"fresh starts organ");
 b1_note_on(&e,60u,0u,120u);
 b1_render_stereo(&e,organ,4096u);
 e.osc_mode=B1_OSC_SAW;
 b1_render_stereo(&e,saw,4096u);
 check(e.osc_blend_q15==32767u,"saw blend reaches full");
 check(memcmp(organ+4096,saw+4096,4096u*sizeof(int16_t))!=0,"saw genuinely changes tone");
 check(energy(saw,8192)>0u,"saw audio present");
 b1_project_capture(&pr,&p,&e);
 check(pr.osc_mode==B1_OSC_SAW && b1_project_valid(&pr),"capture saw in project");
 b1_reset(&e2);b1_performance_init(&p2);
 check(b1_project_apply(&pr,&p2,&e2) && e2.osc_mode==B1_OSC_SAW,"restore saw project");
 e.osc_mode=B1_OSC_ORGAN;
 b1_render_stereo(&e,organ_return,4096u);
 check(e.osc_blend_q15==0u,"return to organ without resetting wheel phase");
 check(e.voice[0].active,"held note survives mode switch");
 b1_project_capture(&pr,&p,&e);
 check(pr.osc_mode==B1_OSC_ORGAN && b1_project_valid(&pr),"organ can be saved");
 pr.osc_mode=B1_OSC_SAW;
 pr.osc_mode=2u;
 check(!b1_project_valid(&pr),"reject unknown oscillator mode");
 struct b1_mode_keys_state k={0};uint8_t t=0;
 uint32_t pp=b1_mode_keys_filter(&k,B1_MODE_HOME,B1_MODE_HOME,100u,&t);
 check(pp==0u&&!t,"defer HOME to allow a chord");
 pp=b1_mode_keys_filter(&k,B1_MODE_FX,B1_MODE_COMBO,138u,&t);
 check(pp==0u&&t,"combo fires with staggered press");
 pp=b1_mode_keys_filter(&k,0u,B1_MODE_COMBO,150u,&t);
 check(pp==0u&&!t,"does not retrigger while held");
 (void)b1_mode_keys_filter(&k,0u,0u,170u,&t);
 pp=b1_mode_keys_filter(&k,B1_MODE_FX,B1_MODE_FX,210u,&t);
 check(pp==0u,"defer standalone FX");
 pp=b1_mode_keys_filter(&k,0u,0u,230u,&t);
 check(pp==B1_MODE_FX&&!t,"standalone FX on release");
 pp=b1_mode_keys_filter(&k,B1_MODE_HOME,B1_MODE_HOME,300u,&t);
 pp=b1_mode_keys_filter(&k,0u,B1_MODE_HOME,400u,&t);
 check(pp==B1_MODE_HOME&&!t,"held HOME still operates");
 printf("B1 V3.3: full saw, return to organ, V6 project padding, HOME+FX chord: PASS\n");
 return 0;
}
