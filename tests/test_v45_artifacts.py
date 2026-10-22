#!/usr/bin/env python3
"""Offline B1 V4.5 release integrity; no physical FM1 claim."""
from pathlib import Path
import hashlib,base64,re,sys
ROOT=Path(__file__).resolve().parent.parent
FW=ROOT/'build/SikkenWorks-B1-V4.5-BEETLE-OTA.fwsc'
PREV=ROOT/'build/SikkenWorks-B1-V4.4-BEETLE-OTA.fwsc'
WEB=ROOT/'B1-WebInstaller.html'
sys.path.insert(0,str(ROOT/'tools'))
from fm1_fwsc import verify_fwsc,parse_ufw
fw=FW.read_bytes();old=PREV.read_bytes();html=WEB.read_text()
report,errors=verify_fwsc(FW)
assert not errors,errors
assert len(fw)==610086 and report['product']=='FM-1_2147',report
m=re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*(\S+)\s*</script>',html)
assert m and base64.b64decode(m.group(1),validate=True)==fw,'Installer embedded firmware mismatch'
sha=hashlib.sha256(fw).hexdigest()
assert f"HASH_ET: '{sha}'" in html and "ID_ET: 'FM-1_2147'" in html
assert 'function isApprovedOtaLoader' in html and "'FM-1_2146'" in html
assert "'FM-1_2147'" in html and 'FM-1_900' in html
flash=lambda x:next(e['data'] for e in parse_ufw(x)['entries'] if e['type']==0)
assert flash(old)[:0x4000]==flash(fw)[:0x4000],'protected boot bytes changed'
assert (ROOT/'firmware/b1_performance.c').read_bytes()==(ROOT/'tests/reference_v41/b1_performance.c').read_bytes(),'performance changed'
assert b'FELUCCA_ID "FM-1_2147"' in (ROOT/'firmware/b1_main.c').read_bytes()
assert b"PRODUCT='FM-1_2147'" in (ROOT/'tools/b1_pack_beetle_ota.py').read_bytes()
assert (ROOT/'tests/native_v45_poly_exact.c').is_file()
print('PASS V4.5: native identity, SHA256, installer match, protected boot unchanged, SLOOP HF2, performance preserved')
print('FWSC SHA256:',sha)
