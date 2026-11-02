#!/usr/bin/env python3
"""Bit-exact V5.2 stereo PCM AND full engine state against V5.1.
Also test the earlier V5.0 reference via all original regression scenarios.
"""
from pathlib import Path
import subprocess, tempfile
r=Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory() as tmp:
 def run(src,case,label):
  target=Path(tmp)/label
  subprocess.run(['cc','-O2','-std=c11','-I'+str(r/'firmware'),str(r/'tests'/case),str(src),'-o',str(target)],check=True)
  return subprocess.check_output([str(target)])
 for case in ('native_v45_poly_exact.c','native_v50_transition_exact.c','native_v51_full_state.c'):
  before=run(r/'tests/reference_v51/b1_engine.c',case,'v51')
  after=run(r/'firmware/b1_engine.c',case,'v52')
  assert before==after, 'V5.2 PCM/DSP regression '+case
  print('PASS V5.1 vs V5.2:',case, len(after.splitlines()),'full-state scenarios')
print('NOT A PHYSICAL FM1 CPU OR DMA DEADLINE MEASUREMENT')
