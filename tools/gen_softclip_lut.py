#!/usr/bin/env python3
"""Generate bounded, monotonic integer LUTs for the B1 organ amplifier.

Only code-generation time involves divisions. Audio callback uses interpolation,
except an overflow fallback far outside ordinary signal levels.
"""
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
out=ROOT/'firmware'/'b1_softclip_lut.h'
step=512; top=1048576
s=['/* SPDX-License-Identifier: GPL-3.0-only. Generated with tools/gen_softclip_lut.py. */',
 '#ifndef B1_SOFTCLIP_LUT_H','#define B1_SOFTCLIP_LUT_H','#include <stdint.h>',
 '/* x ranges 0..1048576 in increments of 512; Q0 amplitudes */']
for knee in (16500,20500,30000):
 vals=[(knee*x)//(knee+x) for x in range(0,top+step,step)]
 s.append('static const uint16_t b1_clip_%d[%d]={'%(knee,len(vals)))
 for start in range(0,len(vals),16):s.append(' '+','.join(str(v) for v in vals[start:start+16])+',')
 s.append('};')
s+=['#endif']
out.write_text('\n'.join(s)+'\n')
print('Generated',out,out.stat().st_size)
