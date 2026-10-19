#!/usr/bin/env python3
"""One-command reproducible B1 V3.6 firmware + installer build.

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
run(py,root/'tools/b1_pack_beetle_ota.py','--reference',root/'build/BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc','--app',root/'build/B1-app.bin','--out',root/'build/SikkenWorks-B1-V3.6-BEETLE-OTA.fwsc')
run(py,root/'tools/build_installer_v36.py')
run(py,root/'tools/fm1_fwsc.py','verify',root/'build/SikkenWorks-B1-V3.6-BEETLE-OTA.fwsc')
run(py,root/'tests/test_v36_release.py')
print('B1 V3.6 full target build, OTA container and WebInstaller verified OFFLINE.')
