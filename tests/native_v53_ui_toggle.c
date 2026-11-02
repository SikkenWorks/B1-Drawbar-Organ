/* SPDX-License-Identifier: GPL-3.0-only. The real firmware UI renderer changes
 * the on-screen AUTO SAVE text when the SAVE preference changes. */
#include "b1_ui.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint16_t a[240*240],b[240*240],c[240*240];
int main(void){
 static struct b1_engine e;
 static struct b1_performance p;
 b1_reset(&e);b1_performance_init(&p);
 struct b1_ui_state s={0};s.audio=&e;s.keys=&p;s.page=4;
 s.project_storage_state=B1_STORAGE_READY;
 b1_ui_render(a,&s);
 s.autosave_enabled=1;b1_ui_render(b,&s);
 s.autosave_pending=1;b1_ui_render(c,&s);
 unsigned diff_ab=0,diff_bc=0;
 for(unsigned i=0;i<240u*240u;i++){diff_ab+=(a[i]!=b[i]);diff_bc+=(b[i]!=c[i]);}
 if(!diff_ab||!diff_bc){fprintf(stderr,"FAILED: SAVE UI toggle/pending not visible\n");return 1;}
 printf("PASS: native SAVE UI AUTO OFF/ON/pending changed %u/%u pixels\n",diff_ab,diff_bc);
 return 0;
}
