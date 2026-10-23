#!/usr/bin/env python3
"""Verified bounds for 256-frame PCM DMA buffering and clickless underrun recovery.
Pure host simulation of the exact integer formulas; does not prove FM1 deadlines.
"""
from pathlib import Path
import re
root=Path(__file__).resolve().parent.parent
s=(root/'firmware/b1_main.c').read_text()
def n(key):
 m=re.search(r'#define\s+'+key+r'\s+(\d+)u',s)
 assert m,key
 return int(m[1])
frames=n('HALF_FRAMES');soft=n('B1_DMA_SOFT_BUDGET_TICKS');hard=n('B1_DMA_HARD_BUDGET_TICKS')
assert frames==256 and 0<soft<hard<frames*24000000//44100
assert 'b1_dma_last_l' in s and 'b1_dma_resume_ramp=1u' in s
assert '((int64_t)b1_dma_last_l*(int64_t)fade)' in s and '((int64_t)b1_dma_last_r*(int64_t)fade)' in s
# Test every legal source DAC PCM (24-bit signed fixed 16 * 128); last positive/negative.
for previous in (-8388608,-32768*128,-16000*128,0,16000*128,32767*128,8388607):
 fade=[int(previous*(frames-1-t)/frames) for t in range(frames)]
 assert abs(fade[0]-previous)<=abs(previous)//frames+1
 assert fade[-1]==0
 assert all(abs(fade[i])>=abs(fade[i+1]) for i in range(frames-1))
 if abs(previous)>32768*128:
  continue  # DAC-wide ramp range, outside the 16-bit DSP output crossfade domain
 for fresh in (-32768,0,32767):
  old=previous//128
  cross=[int((old*(64-t)+fresh*t)/64) for t in range(64)]
  assert cross[0]==old
  assert abs(cross[-1]-fresh)<=abs(fresh-old)//64+1
  assert all(-32768<=v<=32767 for v in cross)
print('PASS: 256-frame DMA buffer, deadline guard, fade-out and 64-frame crossfade bounds')
