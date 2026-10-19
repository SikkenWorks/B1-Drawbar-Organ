#!/usr/bin/env python3
"""B1 V4.1 complete native cross-build and offline verification. No hardware flash."""
from pathlib import Path
import os,subprocess,sys,tempfile
root=Path(__file__).resolve().parent.parent
sdk=Path(os.environ.get('JIELI_TOOLCHAIN',''))
if not os.environ.get('JIELI_TOOLCHAIN') or not (sdk/'pi32v2/bin/clang').is_file():
 sys.exit('Set JIELI_TOOLCHAIN to installed JieLi pi32v2 toolchain')
def run(*a):
 print('RUN',*a,flush=True)
 subprocess.run([str(x) for x in a],cwd=root,check=True)
py=sys.executable
run(py,root/'firmware/build.py')
run(py,root/'tools/fm1_elf_to_app.py',root/'build/B1.elf',root/'build/B1-app.bin','--toolchain',sdk)
fw=root/'build/SikkenWorks-B1-V4.1-BEETLE-OTA.fwsc'
run(py,root/'tools/b1_pack_beetle_ota.py','--reference',root/'build/BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc','--app',root/'build/B1-app.bin','--out',fw)
run(py,root/'tools/build_installer_v41.py')
run(py,root/'tools/fm1_fwsc.py','verify',fw)
run(py,root/'tests/test_v41_release.py')
run('sh',root/'tests/run_v39_quick_tests.sh')
run(py,root/'tests/test_v39_pcm_reference.py')
run('node',root/'tests/test_installer_v41.cjs')
run('node',root/'tests/test_v41_cross_firmware.cjs')
cc=os.environ.get('HOST_CC','cc')
run(cc,'-std=c11','-O2','-Wall','-Wextra','-Werror','-pedantic','-I',root/'firmware',root/'tests/native_v40_key_assign_leds.c',root/'firmware/b1_performance.c',root/'firmware/b1_engine.c','-o',root/'build/native_v40_key_assign_leds')
run(root/'build/native_v40_key_assign_leds')
# Exactly compare FULL organ state, not merely output audio; compare against
# an independent unmodified V4.0 engine when source path was provided.
# The unmodified V4.0 engine is preserved with its GPL header in tests/
# so this release can always run the differential check in isolation.
ref=os.environ.get('B1_V40_SOURCE')
reference_dir=(Path(ref)/'firmware') if ref else (root/'tests/reference_v40')
with tempfile.TemporaryDirectory() as temp:
 a=Path(temp)/'v40';b=Path(temp)/'v41'
 run(cc,'-O3','-std=c11','-I',reference_dir,'-I',root/'firmware',root/'tests/native_v41_idle_compare.c',reference_dir/'b1_engine.c','-o',a)
 run(cc,'-O3','-std=c11','-I',root/'firmware',root/'tests/native_v41_idle_compare.c',root/'firmware/b1_engine.c','-o',b)
 old=subprocess.check_output([str(a)])
 new=subprocess.check_output([str(b)])
 if old!=new: sys.exit('FAIL: V4.1 PCM or internal state differs from V4.0!')
 print('PASS: byte-identical multistage PCM + full DSP state across 4 scenarios')
print('PASS B1 V4.1: native target, FWSC, installer and host regressions. PHYSICAL BOOT UNVERIFIED.')
