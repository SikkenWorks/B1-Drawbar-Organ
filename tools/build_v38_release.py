#!/usr/bin/env python3
"""One-command reproducible B1 V3.8 firmware + installer build.

JIELI_TOOLCHAIN environment variable must point to installed pi32v2 compiler.
Does not flash real FM1 hardware or prove that the loader accepts this package.
"""
from pathlib import Path
import os,subprocess,sys
root=Path(__file__).resolve().parent.parent
sdk=Path(os.environ.get('JIELI_TOOLCHAIN',''))
if not os.environ.get('JIELI_TOOLCHAIN') or not (sdk/'pi32v2/bin/clang').is_file():
 sys.exit('Set JIELI_TOOLCHAIN to a valid JieLi toolchain directory')
def run(*args):
 print('RUN',*args,flush=True)
 subprocess.run(list(map(str,args)),cwd=root,check=True)
py=sys.executable
run(py,root/'firmware/build.py')
run(py,root/'tools/fm1_elf_to_app.py',root/'build/B1.elf',root/'build/B1-app.bin','--toolchain',sdk)
run(py,root/'tools/b1_pack_beetle_ota.py','--reference',root/'build/BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc','--app',root/'build/B1-app.bin','--out',root/'build/SikkenWorks-B1-V3.8-BEETLE-OTA.fwsc')
run(py,root/'tools/build_installer_v38.py')
run(py,root/'tools/fm1_fwsc.py','verify',root/'build/SikkenWorks-B1-V3.8-BEETLE-OTA.fwsc')
run(py,root/'tests/test_v38_release.py')
run('sh',root/'tests/run_v38_quick_tests.sh')
run('node',root/'tests/test_installer_v38.cjs')
run('node',root/'tests/test_v38_cross_firmware.cjs')
# Original software-rendered audition clips for critical listening (not physical FM1 recordings).
cc=os.environ.get('HOST_CC','cc')
run(cc,'-std=c11','-O2','-Wall','-Wextra','-Werror','-pedantic','-I',root/'firmware',root/'tests/render_v38_rock_demo.c',root/'firmware/b1_engine.c','-o',root/'build/render_v38_rock_demo')
run(root/'build/render_v38_rock_demo','ROCK',root/'audio-reference/B1-V3.8-ROCK-DEMO.wav')
run(root/'build/render_v38_rock_demo','B3',root/'audio-reference/B1-V3.8-B3-DEMO.wav')
print('B1 V3.8 full target build, OTA container and WebInstaller verified OFFLINE.')
