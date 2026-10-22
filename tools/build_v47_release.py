#!/usr/bin/env python3
"""Build complete B1 V4.7 native target while enforcing permanent FM-1_2147 ID."""
import os,subprocess,sys
from pathlib import Path
R=Path(__file__).resolve().parent.parent
SDK=Path(os.environ.get('JIELI_TOOLCHAIN',''))
assert (SDK/'pi32v2/bin/clang').is_file(), 'Set JIELI_TOOLCHAIN'
def run(*args):
 print('RUN',*(str(a) for a in args),flush=True)
 subprocess.run(list(map(str,args)),cwd=R,check=True)
assert '#define FELUCCA_ID "FM-1_2147"' in (R/'firmware/b1_main.c').read_text()
assert "PRODUCT='FM-1_2147'" in (R/'tools/b1_pack_beetle_ota.py').read_text()
run(sys.executable,R/'firmware/build.py')
run(sys.executable,R/'tools/fm1_elf_to_app.py',R/'build/B1.elf',R/'build/B1-app.bin','--toolchain',SDK)
run(sys.executable,R/'tools/b1_pack_beetle_ota.py','--reference',R/'build/BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc','--app',R/'build/B1-app.bin','--out',R/'build/SikkenWorks-B1-V4.7-BEETLE-OTA.fwsc')
run(sys.executable,R/'tools/build_installer_v47.py')
print('B1 V4.7 native build complete: ID FM-1_2147 PRESERVED')
