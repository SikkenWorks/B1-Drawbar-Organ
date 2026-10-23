#!/usr/bin/env python3
"""Repeat all B1 V4.8 release checks (no physical FM1 connection)."""
import subprocess,sys
from pathlib import Path
R=Path(__file__).resolve().parent.parent
def run(*cmd):
 print('RUN',*map(str,cmd),flush=True)
 subprocess.run([str(x) for x in cmd],cwd=R,check=True)
run(sys.executable,R/'tests/test_v48_release.py')
run('node',R/'tests/test_installer_v48.cjs')
run(sys.executable,R/'tests/test_v39_pcm_reference.py')
print('PASS: B1 V4.8 native build and package consistency, native replay, PCM, DMA guard, WebInstaller JS verified (not physical flashing).')
