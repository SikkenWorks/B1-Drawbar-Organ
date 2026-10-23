#!/usr/bin/env python3
"""Reproducible V4.8 artifact and PCM integrity checks; never flashes FM1."""
from pathlib import Path
import sys,base64,hashlib,subprocess,tempfile,re
R=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(R/'tools'))
from fm1_fwsc import verify_fwsc,parse_ufw
fw=R/'build/SikkenWorks-B1-V4.8.fwsc'
data=fw.read_bytes()
report,errors=verify_fwsc(fw)
assert not errors and report['valid'] and report['product']=='FM-1_481026',(report,errors)
sha=hashlib.sha256(data).hexdigest()
assert '#define FELUCCA_ID "FM-1_481026"' in (R/'firmware/b1_main.c').read_text()
assert "PRODUCT='FM-1_481026'" in (R/'tools/b1_pack_beetle_ota.py').read_text()
html=(R/'B1-WebInstaller.html').read_text()
assert "ID_ET: 'FM-1_481026'" in html
assert f"HASH_ET: '{sha}'" in html
assert base64.b64encode(data).decode() in html
assert 'V4.8' in html
assert '<script id="firmware-embedded"' in html
assert 'function isApprovedOtaLoader' in html
assert 'ota-FM-1_' in html
# No accidental replacement by BEETLE product in target container
pkg=parse_ufw(data)
assert pkg['product']=='FM-1_481026'
assert 'FM-1_2147' not in (R/'firmware/b1_main.c').read_text()
ref=R/'build/BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc'
assert parse_ufw(ref.read_bytes())['product']=='FM-1_2101'
print('PASS: app ID, FWSC ID, new identity vs pinned BEETLE boot reference, UI embedded bytes/hash')
# Native FM1 app address range is checked by packaging code, and compile output exists.
assert (R/'build/B1.elf').is_file() and (R/'build/B1-app.bin').is_file()
print('PASS: native pi32v2 ELF and app.bin present, UFW/application/loader CRC verified')
with tempfile.TemporaryDirectory() as d:
 def run_c(test,output,src=None):
  exe=Path(d)/output
  args=['cc','-std=c11','-O2','-I'+str(R/'firmware'),str(R/'tests'/test),str(src if src else R/'firmware/b1_engine.c'),'-o',str(exe)]
  subprocess.run(args,check=True,stdout=subprocess.DEVNULL)
  return subprocess.check_output([str(exe)])
 # V4.7 and V4.8 engine byte-for-byte same: full state/PCM must match 96 tests
 a=run_c('native_v45_poly_exact.c','reference',R/'tests/reference_v46/b1_engine.c')
 b=run_c('native_v45_poly_exact.c','current')
 assert a==b
 print('PASS: 96 1..12 voice PCM + entire DSP state reference cases')
 print(run_c('test_v48_chunk_consistency.c','chunk').decode().strip())
 print(run_c('test_v48_poly_output.c','stress').decode().strip())
subprocess.run([sys.executable,str(R/'tests/test_v48_dma_policy.py')],check=True)
subprocess.run([sys.executable,str(R/'tests/run_ota_v48_replay.py'),'cc'],check=True,stdout=subprocess.DEVNULL)
print('PASS: 16 native CHECK->staging->success->commit and rejection replays')
print('NOT PROVEN: physical B1->Felucca->B1, physical CPU timing, crackle-free 3+ voices')
