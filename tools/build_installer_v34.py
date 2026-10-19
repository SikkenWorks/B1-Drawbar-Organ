#!/usr/bin/env python3
"""Idempotently embed exactly the audited B1 V3.4 .fwsc. Do not weaken OTA checks."""
from pathlib import Path
import re, hashlib, base64
root=Path(__file__).resolve().parent.parent
html=root/'B1-WebInstaller.html'
fwpath=root/'build/SikkenWorks-B1-V3.4-BEETLE-OTA.fwsc'
fw=fwpath.read_bytes()
if len(fw)!=610086: raise SystemExit('Unexpected .fwsc length, refused')
sha=hashlib.sha256(fw).hexdigest()
s=html.read_text()
s=s.replace('B1 V3.3','B1 V3.4').replace('B1-V3.3','B1-V3.4')
s=re.sub(r"(ID_ET: ')[^']+(')",r'\g<1>FM-1_2134\2',s,count=1)
s=re.sub(r"(HASH_ET: ')[0-9a-f]{64}(')",lambda m:m[1]+sha+m[2],s,count=1)
s=re.sub(r'(<script id="firmware-embedded" type="application/octet-stream">)\s*.*?\s*(</script>)',lambda m:m[1]+'\n'+base64.b64encode(fw).decode()+'\n'+m[2],s,count=1,flags=re.S)
if 'FM-1_2134' not in s: raise SystemExit('Firmware ID not embedded')
html.write_text(s)
print('Installer B1 V3.4:', sha, len(fw))
