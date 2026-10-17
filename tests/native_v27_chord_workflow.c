/* B1 V2.7: default chord-off, physical assignment, toggle without erasing maps,
 * note routing and project V6 backward compatibility with old padding bytes. */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "b1_performance.h"
int main(void){
 struct b1_engine e;struct b1_performance p;
 b1_reset(&e);b1_performance_init(&p);
 assert(sizeof(struct b1_project)==124u);
 assert(offsetof(struct b1_project,checksum)==120u);
 assert(!b1_chord_enabled(&p));
 assert(!b1_assign_chord_key(&p,21u,B1_MIN7));
 assert(b1_set_chord_enabled(&p,&e,1u));
 assert(b1_chord_enabled(&p));
 assert(p.range[2].type==B1_CHORD&&p.range[2].lo==20u&&p.range[2].hi==26u);
 assert(!b1_assign_chord_key(&p,4u,B1_MIN7));
 assert(b1_assign_chord_key(&p,21u,B1_MIN7));
 assert(p.chord_map[21u]==B1_MIN7);
 assert(b1_set_chord_attack(&p,73u));
 assert(b1_chord_attack(&p)==73u);
 b1_performance_keys(&p,&e,(1u<<21u));
 assert(b1_active_chord(&p)==B1_MIN7);
 b1_performance_keys(&p,&e,(1u<<21u)|(1u<<12u));
 assert(p.held[12u].count==4u);
 assert(b1_set_chord_enabled(&p,&e,0u));
 assert(!b1_chord_enabled(&p));
 assert(!b1_assign_chord_key(&p,21u,B1_MAJ));
 b1_performance_keys(&p,&e,1u<<21u);
 assert(p.held[21u].count==1u); /* normal SOLO/ORGAN when disabled */
 assert(b1_set_chord_enabled(&p,&e,1u));
 assert(p.chord_map[21u]==B1_MIN7);
 assert(b1_chord_attack(&p)==73u);
 struct b1_project project;
 b1_project_capture(&project,&p,&e);
 assert(project.chord_mode==2u && b1_project_valid(&project));
 struct b1_engine re;struct b1_performance rp;
 b1_reset(&re);b1_performance_init(&rp);
 assert(b1_project_apply(&project,&rp,&re));
 assert(b1_chord_enabled(&rp));assert(rp.range[2].attack==73u);
 assert(rp.chord_map[21u]==B1_MIN7);
 assert(b1_set_chord_enabled(&rp,&re,0u));
 b1_project_capture(&project,&rp,&re);
 assert(project.chord_mode==1u);
 b1_performance_init(&rp);assert(b1_project_apply(&project,&rp,&re));
 assert(!b1_chord_enabled(&rp));assert(rp.chord_map[21u]==B1_MIN7);
 /* Legacy schema 6 archives have zeros in the formerly unused padding byte. */
 project.chord_mode=0u;
 const unsigned char *ptr=(const unsigned char*)&project;
 uint32_t h=2166136261u;
 for(size_t i=0;i<offsetof(struct b1_project,checksum);i++){h^=ptr[i];h*=16777619u;}
 project.checksum=h;
 assert(b1_project_valid(&project));
 b1_performance_init(&rp);assert(b1_project_apply(&project,&rp,&re));
 assert(b1_chord_enabled(&rp));
 /* Full-slot safety: never overwrite an organ range just to enable chords. */
 b1_performance_init(&rp);
 for(uint8_t i=0;i<B1_RANGES;i++)rp.range[i].type=B1_ORGAN;
 assert(!b1_set_chord_enabled(&rp,&re,1u));
 puts("B1 V2.7: chord editor toggle, physical selection, human attack, V6 project compatibility PASS");
 return 0;
}
