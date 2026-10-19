#!/usr/bin/env python3
"""Rebuild the native B1 V4.4 OTA-repaired firmware. Does not flash a device."""
from pathlib import Path
import os,subprocess,sys,hashlib,re,base64
root=Path(__file__).resolve().parent.parent
sdk=Path(os.environ.get('JIELI_TOOLCHAIN',''))
if not (sdk/'pi32v2/bin/clang').is_file():raise SystemExit('Set JIELI_TOOLCHAIN to JieLi pi32v2')
fw=root/'build/SikkenWorks-B1-V4.4-BEETLE-OTA.fwsc'
def run(*a):subprocess.run([str(x) for x in a],cwd=root,check=True)
run(sys.executable,root/'firmware/build.py')
run(sys.executable,root/'tools/fm1_elf_to_app.py',root/'build/B1.elf',root/'build/B1-app.bin','--toolchain',sdk)
run(sys.executable,root/'tools/b1_pack_beetle_ota.py','--reference',root/'build/BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc','--app',root/'build/B1-app.bin','--out',fw)
run(sys.executable,root/'tools/build_installer_v44.py')
run(sys.executable,root/'tests/run_ota_v44_replay.py')
run(sys.executable,root/'tests/test_v44_artifacts.py')
print('PASS: V4.4 native app, audited FWSC, SLOOP-HF2 installer, and native packet replay')
