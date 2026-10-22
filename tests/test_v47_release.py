#!/usr/bin/env python3
"""Repeatable V4.7 lossless audio / source release / OTA regression checks.

Tests compare original V4.6 engine PCM + complete DSP state against
V4.7. Host timing is NOT a hardware benchmark.
"""
import hashlib,subprocess,sys,tempfile,base64,re
from pathlib import Path
R=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(R/'tools'))
from fm1_fwsc import verify_fwsc
fw=R/'build/SikkenWorks-B1-V4.7-BEETLE-OTA.fwsc'
raw=fw.read_bytes();report,errors=verify_fwsc(fw)
assert report['valid'] and not errors and report['product']=='FM-1_2147'
sha=hashlib.sha256(raw).hexdigest()
html=(R/'B1-WebInstaller.html').read_text()
assert "ID_ET: 'FM-1_2147'" in html and f"HASH_ET: '{sha}'" in html
assert base64.b64encode(raw).decode() in html
main=(R/'firmware/b1_main.c').read_text()
assert '#define FELUCCA_ID "FM-1_2147"' in main
usb=(R/'firmware/usb.c').read_text()
assert 'static const uint8_t STR1[] = {24, 3' in usb
assert "'F', 0, 'M', 0, '-', 0, '1', 0" in usb
print('PASS package SHA, fixed identity, USB MIDI product, installer embedded firmware')
with tempfile.TemporaryDirectory() as d:
 d=Path(d)
 def build(source,name):
  exe=d/name
  subprocess.run(['cc','-std=c11','-O2','-I'+str(R/'firmware'),str(R/'tests/native_v45_poly_exact.c'),str(source),'-o',str(exe)],check=True)
  return subprocess.check_output([str(exe)])
 assert build(R/'tests/reference_v46/b1_engine.c','v46')==build(R/'firmware/b1_engine.c','v47')
print('PASS 96 exact PCM + entire DSP state signatures (1–12 voices, 8 settings)')
subprocess.run([sys.executable,str(R/'tests/run_ota_v47_replay.py')],check=True,stdout=subprocess.DEVNULL)
print('PASS 16 native CHECK/stage/success/rollback OTA packet replays')
print('PASS all V4.7 software checks (FM1 physical boot/CPU/DMA remain untested)')
