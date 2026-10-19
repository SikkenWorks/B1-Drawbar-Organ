#!/usr/bin/env python3
"""Pin the exact native B1 V4.4 firmware in the proven SLOOP-compatible HF2 installer."""
import base64,hashlib,re
from pathlib import Path
root=Path(__file__).resolve().parent.parent
fw=(root/'build/SikkenWorks-B1-V4.4-BEETLE-OTA.fwsc').read_bytes()
if len(fw)!=610086:raise SystemExit('Unexpected .fwsc length')
sha=hashlib.sha256(fw).hexdigest()
s=(root/'B1-WebInstaller.html').read_text()
if "ID_ET: 'FM-1_2145'" not in s and "ID_ET: 'FM-1_2146'" not in s:raise SystemExit('Expected HF2 B1 V4.3/V4.4 installer')
if 'function isApprovedOtaLoader' not in s or 'FM-1_900' not in s:raise SystemExit('Missing SLOOP HF2 support')
s=s.replace('B1 V4.3','B1 V4.4').replace('B1-V4.3','B1-V4.4')
s,n=re.subn(r"(HASH_ET: ')[0-9a-f]{64}(')",lambda m:m[1]+sha+m[2],s,count=1)
if n!=1:raise SystemExit('HASH_ET replacement failed')
s=s.replace("ID_ET: 'FM-1_2145'", "ID_ET: 'FM-1_2146'")
s=s.replace("  'FM-1_2145'\n]);", "  'FM-1_2145',\n  'FM-1_2146'\n]);")
s,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">)\s*.*?\s*(</script>)',lambda m:m[1]+'\n'+base64.b64encode(fw).decode()+'\n'+m[2],s,flags=re.S,count=1)
if n!=1:raise SystemExit('Firmware embedding failed')
if f"HASH_ET: '{sha}'" not in s or "ID_ET: 'FM-1_2146'" not in s:raise SystemExit('Identity/hash mismatch')
(root/'B1-WebInstaller.html').write_text(s)
print('PASS: SLOOP-HF2 B1 V4.4 installer, embedded FWSC SHA-256',sha)
