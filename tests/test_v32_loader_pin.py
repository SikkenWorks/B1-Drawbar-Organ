#!/usr/bin/env python3
"""Verify exact compiled OTA loader and both retry-safe application flash checks."""
from pathlib import Path
import sys,re,hashlib
root=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(root/'tools'))
from fm1_fwsc import parse_ufw
ref=(root/'build/BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc').read_bytes()
actual=(root/'build/SikkenWorks-B1-V3.2-BEETLE-OTA.fwsc').read_bytes()
source=next(e['data'] for e in parse_ufw(ref)['entries'] if e['name']=='ota.bin')[32:]
target=next(e['data'] for e in parse_ufw(actual)['entries'] if e['name']=='ota.bin')[32:]
arr=(root/'firmware/b1_pinned_loader.h').read_text()
compiled=bytes(int(h,16) for h in re.findall(r'0x([0-9a-f]{2})',arr))
assert len(compiled)==6898
assert compiled==source==target
assert hashlib.sha256(target).hexdigest()=='d0b1a05c63030b58e5be06b93751e6cc375d3d34eec584109f97fd31f0a42a78'
code=(root/'firmware/ota.c').read_text()
assert 'ota_memeq(b,b1_trusted_loader+i,n)' in code
assert 'return -18; /* staged flash readback mismatch */' in code
assert 'if(!etch_ota_hash(ota_off+32,b,1))return -16;' in code
print('PASS exact compiled loader body, host preflight, post-flash readback, -16/-18 distinct')
