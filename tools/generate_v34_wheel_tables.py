#!/usr/bin/env python3
"""Deterministic ROM tables: exact V3.3 tonewheel shaping and SAFE Leslie tremolo.
Values computed once during the *build*, never inside the 44.1 kHz audio ISR.
"""
from pathlib import Path
import re
root=Path(__file__).resolve().parent.parent
src=(root/'firmware'/'b1_tables.h').read_text()
m=re.search(r'b1_sine\[2048\]\s*=\s*\{([^}]*)\}',src,re.S)
assert m
sine=[int(v) for v in re.findall(r'-?\d+',m.group(1))]
assert len(sine)==2048
# C99 integer division truncates toward zero, unlike Python // for negatives
trunc=lambda x,y: abs(x)//y*(1 if x>=0 else -1)
even=[(w*7+11)%21-10 for w in range(91)]
odd=[(w*11+5)%19-9 for w in range(91)]
gain=[246+(w*13+7)%21 for w in range(91)]
breathe=[trunc(s,1100) for s in sine]
assert min(breathe)>=-127 and max(breathe)<=127
header=['/* SPDX-License-Identifier: GPL-3.0-only */',
 '/* B1 V3.4 ROM lookup values, exactly equivalent to V3.3 per-sample expressions. */',
 '#ifndef B1_V34_SHAPING_TABLES_H', '#define B1_V34_SHAPING_TABLES_H', '#include <stdint.h>']
for name,ctype,vals in [('b1_wheel_even','int8_t',even),('b1_wheel_odd','int8_t',odd),('b1_wheel_gain','uint16_t',gain),('b1_rotor_breathe','int8_t',breathe)]:
 header.append(f'static const {ctype} {name}[{len(vals)}] = {{')
 for start in range(0,len(vals),24):header.append(' '+', '.join(map(str,vals[start:start+24]))+(',' if start+24<len(vals) else '') )
 header.append('};')
header.append('#endif')
(root/'firmware'/'b1_v34_shaping_tables.h').write_text('\n'.join(header)+'\n')
print('Generated',len(breathe),'Leslie breath and 91 x 3 tonewheel shape constants')
