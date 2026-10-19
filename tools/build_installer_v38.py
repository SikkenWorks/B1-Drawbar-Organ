#!/usr/bin/env python3
"""Embed the newly target-compiled V3.8 firmware in the existing B1 installer.

Retain all cryptographic safety checks, protected boot header and known-source
whitelist, only append the positively identified latest B1 version.
"""
from pathlib import Path
import base64, hashlib, re
root=Path(__file__).resolve().parent.parent
html=root/'B1-WebInstaller.html'
fw=(root/'build/SikkenWorks-B1-V3.8-BEETLE-OTA.fwsc').read_bytes()
if len(fw)!=610086:raise SystemExit('Unrecognized FWSC size')
sha=hashlib.sha256(fw).hexdigest()
s=html.read_text()
if "ID_ET: 'FM-1_2137'" in s:
 if "  'FM-1_2137'\n]);" not in s:raise SystemExit('Unexpected approved sources')
 s=s.replace('B1 V3.7','B1 V3.8').replace('B1-V3.7','B1-V3.8')
 s=s.replace("  'FM-1_2137'\n]);", "  'FM-1_2137',\n  'FM-1_2138'\n]);")
 s=s.replace('SikkenWorks B1 V3.8, FM-1_2137.', 'SikkenWorks B1 V3.8, FM-1_2138.')
elif "ID_ET: 'FM-1_2138'" in s:
 if "  'FM-1_2137',\n  'FM-1_2138'\n]);" not in s:
  raise SystemExit('V3.8 already-built installer is missing previous supported sources')
else:raise SystemExit('Expected V3.7 or V3.8 B1 WebInstaller')
s,n=re.subn(r"(HASH_ET: ')[0-9a-f]{64}(')",lambda m:m[1]+sha+m[2],s,count=1)
assert n==1
s,n=re.subn(r"(ID_ET: ')[^']+(')",lambda m:m[1]+'FM-1_2138'+m[2],s,count=1)
assert n==1
s,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">)\s*.*?\s*(</script>)',
 lambda m:m[1]+'\n'+base64.b64encode(fw).decode()+'\n'+m[2],s,count=1,flags=re.S)
assert n==1
assert "'FM-1_2137'" in s and "'FM-1_2138'" in s
assert "ID_ET: 'FM-1_2138'" in s and 'B1 V3.7' not in s
html.write_text(s)
print('Embedded V3.8 FWSC',len(fw),sha)
