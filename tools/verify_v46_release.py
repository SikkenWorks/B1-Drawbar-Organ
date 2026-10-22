#!/usr/bin/env python3
"""Run source/target artifacts and production native CHECK/commit plus PCM tests.
No hardware is connected; this runner never flashes an FM1.
"""
import subprocess,sys
from pathlib import Path
R=Path(__file__).resolve().parent.parent

def run(*cmd):
 print('RUN',*(str(x) for x in cmd),flush=True)
 subprocess.run([str(x) for x in cmd],cwd=R,check=True)
run(sys.executable,R/'tests/test_v46_artifacts.py')
run('node',R/'tests/test_installer_v46.cjs')
run(sys.executable,R/'tests/run_ota_v46_replay.py','cc')
run(sys.executable,R/'tests/test_v39_pcm_reference.py')
run('bash',R/'tests/run_v39_quick_tests.sh')
print('PASS: V4.6 OTA/installer/integrity/PCM/native functional regression (hardware not connected).')
