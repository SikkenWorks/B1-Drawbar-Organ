#!/usr/bin/env python3
"""Re-pin exactly the B1 V4.8 OTA bytes while keeping FM-1_481026.
No firmware ID cycling in this or any future build."""
import base64, hashlib, re
from pathlib import Path
root=Path(__file__).resolve().parent.parent
fw=root/'build/SikkenWorks-B1-V4.8.fwsc'
raw=fw.read_bytes(); sha=hashlib.sha256(raw).hexdigest()
if len(raw)!=610086: raise SystemExit('Unexpected FWSC length')
s=(root/'B1-WebInstaller.html').read_text()
if 'function isApprovedOtaLoader' not in s or 'FM-1_900' not in s: raise SystemExit('Missing proven SLOOP HF2 path')
if "ID_ET: 'FM-1_481026'" not in s: raise SystemExit('Identity MUST remain FM-1_481026')
s=s.replace('B1 V4.5','B1 V4.8').replace('B1-V4.5','B1-V4.8')
s,n=re.subn(r"(HASH_ET: ')[0-9a-f]{64}(')",lambda m:m[1]+sha+m[2],s,count=1)
if n!=1: raise SystemExit('HASH_ET replacement failed')
s,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">)\s*.*?\s*(</script>)',lambda m:m[1]+'\n'+base64.b64encode(raw).decode()+'\n'+m[2],s,flags=re.S,count=1)
if n!=1: raise SystemExit('Firmware embedding failed')
if f"HASH_ET: '{sha}'" not in s: raise SystemExit('Hash mismatch')
(root/'B1-WebInstaller.html').write_text(s)
print('PASS: V4.8 WebInstaller; product still FM-1_481026; SHA256',sha)
