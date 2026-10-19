/* GPL-3.0-only: Native HOST idle performance measurement, not FM1 timing. */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "b1_engine.h"
static struct b1_engine e;
static int16_t out[256];
static uint64_t ns(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000000u+(uint64_t)t.tv_nsec;}
int main(void){
 b1_reset(&e);
 for(unsigned i=0;i<130;i++)b1_render_stereo(&e,out,128u);
 uint64_t start=ns();
 for(unsigned i=0;i<24000u;i++)b1_render_stereo(&e,out,128u);
 uint64_t end=ns();
 printf("Idle 3,072,000 frames: %llu ns total, %.3f ns/frame (host)\n",
 (unsigned long long)(end-start), (double)(end-start)/3072000.0);
 /* Prevent stale PCM optimized away from caller at program termination. */
 printf("PCM %d %d, wheel frame %u\n",out[0],out[1],e.wheel_frame);
 return 0;
}
