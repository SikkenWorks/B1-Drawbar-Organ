#!/usr/bin/env python3
"""Reproducible native B1 V4.5 firmware and host regressions. Never flashes hardware."""
import os,subprocess,sys,tempfile,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
SDK=Path(os.environ.get('JIELI_TOOLCHAIN',''))
if not (SDK/'pi32v2/bin/clang').is_file():sys.exit('Set JIELI_TOOLCHAIN to installed JieLi pi32v2')
CC=os.environ.get('HOST_CC','cc')
def run(*cmd):
 print('RUN',*(str(x) for x in cmd),flush=True)
 subprocess.run([str(x) for x in cmd],cwd=ROOT,check=True)
py=sys.executable
run(py,ROOT/'firmware/build.py')
run(py,ROOT/'tools/fm1_elf_to_app.py',ROOT/'build/B1.elf',ROOT/'build/B1-app.bin','--toolchain',SDK)
fw=ROOT/'build/SikkenWorks-B1-V4.5-BEETLE-OTA.fwsc'
run(py,ROOT/'tools/b1_pack_beetle_ota.py','--reference',ROOT/'build/BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc','--app',ROOT/'build/B1-app.bin','--out',fw)
run(py,ROOT/'tools/build_installer_v45.py')
run(py,ROOT/'tests/test_v45_artifacts.py')
run(py,ROOT/'tests/test_v39_pcm_reference.py')
run('bash',ROOT/'tests/run_v39_quick_tests.sh')
run(py,ROOT/'tests/run_ota_v44_replay.py',CC,fw)
with tempfile.TemporaryDirectory(prefix='b1-v45-') as tmp:
 tmp=Path(tmp)
 inc=['-std=c11','-O3','-I',ROOT/'firmware']
 def compile_test(name,source):
  bin=tmp/name
  run(CC,*inc,ROOT/'tests/native_v45_poly_exact.c',source,'-o',bin)
  return subprocess.check_output([str(bin)])
 original=compile_test('reference',ROOT/'tests/reference_v41/b1_engine.c')
 current=compile_test('v45',ROOT/'firmware/b1_engine.c')
 if original!=current:sys.exit('FAIL: polyphonic PCM or full DSP state mismatch vs V4.4')
 print('PASS 96 scenarios: 1..12 voices, 8 configs; PCM and entire DSP state byte-identical')
 a=tmp/'native_v45_exact_percussion'
 run(CC,*inc,ROOT/'tests/native_v45_exact_percussion.c','-o',a)
 run(a)
 a=tmp/'native_v40_key_assign_leds'
 run(CC,*inc,ROOT/'tests/native_v40_key_assign_leds.c',ROOT/'firmware/b1_performance.c',ROOT/'firmware/b1_engine.c','-o',a)
 run(a)
 a=tmp/'reference_idle';b=tmp/'v45_idle'
 for target,source in [(a,ROOT/'tests/reference_v41/b1_engine.c'),(b,ROOT/'firmware/b1_engine.c')]:
  run(CC,*inc,ROOT/'tests/native_v41_idle_compare.c',source,'-o',target)
 if subprocess.check_output([str(a)])!=subprocess.check_output([str(b)]):sys.exit('FAIL: idle full-state mismatch')
 print('PASS idle PCM + entire DSP state byte-identical vs V4.4')
print('PASS: B1 V4.5 native firmware + host regressions. PHYSICAL CPU/DMA PERF UNVERIFIED.')
