/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "b1_master_gain.h"
static uint32_t seed=0x71895a23u;
static uint32_t rnd(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
int main(void){
    static const uint16_t gains[]={0,1,2,127,255,256,511,1023,4095,8191,16383,32766,32767};
    unsigned long long checked=0;
    for(unsigned j=0;j<sizeof(gains)/sizeof(*gains);++j){
        for(int pcm=-32768;pcm<=32767;++pcm){
            int32_t ref=(pcm*(int32_t)gains[j])/32767;
            int32_t got=b1_master_gain_exact((int16_t)pcm,gains[j]);
            if(ref!=got){fprintf(stderr,"MISMATCH pcm=%d gain=%u ref=%ld got=%ld\n",pcm,(unsigned)gains[j],(long)ref,(long)got);return 1;}
            ++checked;
        }
    }
    for(unsigned k=0;k<1000000u;++k){
        int16_t pcm=(int16_t)rnd();uint16_t gain=(uint16_t)(rnd() & 32767u);
        int32_t ref=((int32_t)pcm*gain)/32767;
        assert(ref==b1_master_gain_exact(pcm,gain));
        ++checked;
    }
    printf("PASS: %llu exact signed PCM/gain equivalence cases, all output integers match\n",checked);
}
