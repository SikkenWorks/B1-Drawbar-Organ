/* GPL-3.0-only. Exhaustive edge + randomized differential test of the native
 * fast percussion divider against the original signed 64-bit formula. */
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include "b1_engine.c" /* test-only access to static b1_percussion_bus_exact */
static uint32_t r=0x19da1605u;
static uint32_t random32(void){r=1664525u*r+1013904223u;return r;}
static int check(int32_t bus,uint32_t env){
 int32_t expect=(int32_t)(((int64_t)bus*env)/260000u);
 int32_t got=b1_percussion_bus_exact(bus,env);
 if(got!=expect){fprintf(stderr,"FAIL bus=%ld env=%lu got=%ld expected=%ld\n",(long)bus,(unsigned long)env,(long)got,(long)expect);return 0;}
 return 1;
}
int main(void){
 const int32_t edge[]={INT32_MIN,-2000001,-2000000,-65539,-65538,-65537,-65536,-260000,-1,0,1,65536,65537,65538,65539,260000,2000000,2000001,INT32_MAX};
 const uint32_t envs[]={0,1,2,31,32,33,255,1024,8124,8125,9999,32766,32767,32768,65535};
 for(unsigned a=0;a<sizeof(edge)/sizeof(edge[0]);a++)for(unsigned b=0;b<sizeof(envs)/sizeof(envs[0]);b++)if(!check(edge[a],envs[b]))return 1;
 for(unsigned i=0;i<1000000u;i++){
  int32_t bus=(int32_t)(random32()%4000001u)-2000000;
  uint32_t env=random32()%32768u;
  if(!check(bus,env)||!check(-bus,env))return 2;
 }
 puts("PASS: 2,000,285 percussion-divider input pairs match the original 64-bit formula exactly");
}
