#include "../firmware/b1_startup.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
static uint16_t fb[240u*240u];
static struct b1_engine eng;
static struct b1_performance perf;
static int state[41];
static void capture(uint32_t id,int on){if(id<41)state[id]=on;}
static int require(int condition,const char *why){if(!condition){fprintf(stderr,"FAIL: %s\n",why);return 0;}return 1;}
static uint64_t digest(const uint16_t *p){uint64_t h=1469598103934665603ULL;
 for(unsigned i=0;i<240*240;i++){h^=p[i];h*=1099511628211ULL;}return h;}
int main(void){
 int ok=1;struct b1_ui_state h={0};h.audio=&eng;h.keys=&perf;
 b1_reset(&eng);b1_performance_init(&perf);
 b1_startup_render(fb,&h,0u);uint64_t start=digest(fb);
 b1_startup_render(fb,&h,600u);uint64_t forming=digest(fb);
 b1_startup_render(fb,&h,1510u);uint64_t held=digest(fb);
 b1_startup_render(fb,&h,2600u);uint64_t held2=digest(fb);
 b1_startup_render(fb,&h,3140u);uint64_t transitioning=digest(fb);
 b1_startup_render(fb,&h,3220u);
 uint16_t home[240u*240u];b1_ui_render(home,&h);
 ok &= require(start!=forming,"particles must appear");
 ok &= require(forming!=held,"logo must manifest into final mask");
 ok &= require(held==held2,"1.6 second logo hold must stay absolutely still");
 ok &= require(held!=transitioning,"dissolve must be visible");
 ok &= require(memcmp(fb,home,sizeof(fb))==0,"last startup frame must match actual HOME framebuffer");
 ok &= require(B1_STARTUP_HOLD_END_MS-B1_STARTUP_HOLD_BEGIN_MS==1600u,"1.6 second logo hold");
 b1_startup_leds(200u,capture);
 ok &= require(state[14+13]&&state[14+14],"two centre note LEDs must light first");
 ok &= require(!state[14]&&!state[14+26],"edge LEDs must be dark at first ripple");
 int seen[41]={0};for(uint32_t t=0;t<=1500;t+=10){b1_startup_leds(t,capture);for(int i=0;i<41;i++)seen[i]|=state[i];}
 for(int i=0;i<41;i++)ok &= require(seen[i],"ALL 41 key/button LEDs must be reached by ripple");
 b1_startup_leds(B1_STARTUP_DURATION_MS,capture);
 for(int i=0;i<41;i++)ok &= require(!state[i],"startup must release LEDs to regular policy");
 if(!ok)return 1;
 printf("B1 V1.3: exact startup logo, hold, dissolve, HOME, 41-LED ripple PASS\n");return 0;
}
