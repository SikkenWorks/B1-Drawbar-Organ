#!/usr/bin/env python3
"""Native host equality check against the untouched V5.0 DSP engine.
Independent of OTA or hardware timing; compare all PCM plus full state.
"""
from pathlib import Path
import subprocess, tempfile
r=Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory() as td:
 def compile_and_run(src,name,case):
  executable=Path(td)/name
  subprocess.run(['cc','-O2','-std=c11','-I'+str(r/'firmware'),
   str(r/'tests'/case),str(src),'-o',str(executable)],check=True)
  return subprocess.check_output([str(executable)])
 for case in ('native_v45_poly_exact.c','native_v50_transition_exact.c','native_v51_full_state.c'):
  before=compile_and_run(r/'tests/reference_v50/b1_engine.c','old',case)
  after=compile_and_run(r/'firmware/b1_engine.c','new',case)
  assert before==after,case+' produced changed PCM or DSP state'
  print('PASS:',case,'exact V5.0 PCM + full-engine-state match,',len(after.splitlines()),'cases')
print('Headroom requires physical AUDIO DIAG measurements; host PCM equivalence is not a timing measurement.')
