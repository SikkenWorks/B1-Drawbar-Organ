/* B1 V2.4 factory rotor defaults and persisted/user project preservation. */
#include "../firmware/b1_engine.h"
#include "../firmware/b1_performance.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 static struct b1_engine eng;
 static struct b1_performance perf;
 static struct b1_project project;
 b1_reset(&eng);b1_performance_init(&perf);
 assert(eng.rotor_enabled==1u);
 assert(eng.rotor_fast==1u);
 assert(eng.rotor_mix==60u);
 assert(eng.rotor_inc==612000u && eng.drum_inc==573000u);
 /* A user's previously stored rotor settings override factory defaults. */
 eng.rotor_fast=0u;eng.rotor_mix=23u;
 b1_project_capture(&project,&perf,&eng);
 b1_reset(&eng);assert(eng.rotor_fast==1 && eng.rotor_mix==60);
 assert(b1_project_apply(&project,&perf,&eng));
 assert(eng.rotor_fast==0u && eng.rotor_mix==23u);
 b1_reset(&eng);assert(eng.rotor_fast==1u && eng.rotor_mix==60u);
 puts("B1 V2.4: rotor FAST/MIX60 factory reset; existing project values preserved PASS");
 return 0;
}
