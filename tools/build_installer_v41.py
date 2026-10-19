#!/usr/bin/env python3
"""Embed exact freshly compiled B1 V4.1 firmware; keep V4.0 upgrade allowed.

DO NOT relax SHA-256, BEETLE header, stock recovery or device identity checks.
"""
from pathlib import Path
import base64,hashlib,re
root=Path(__file__).resolve().parent.parent
html=root/'B1-WebInstaller.html'
fw=(root/'build/SikkenWorks-B1-V4.1-BEETLE-OTA.fwsc').read_bytes()
if len(fw)!=610086: raise SystemExit('Unexpected V4.1 .fwsc size')
sha=hashlib.sha256(fw).hexdigest()
s=html.read_text()
if "ID_ET: 'FM-1_2142'" in s:
 if "  'FM-1_2142'\n]);" not in s: raise SystemExit('V4.0 source allowlist unexpected')
 s=s.replace('B1 V4.0','B1 V4.1').replace('B1-V4.0','B1-V4.1')
 s=s.replace("  'FM-1_2142'\n]);", "  'FM-1_2142',\n  'FM-1_2143'\n]);")
 s=s.replace('FM-1_2142. Built-in exact pinned', 'FM-1_2143. Built-in exact pinned')
elif "ID_ET: 'FM-1_2143'" in s:
 if "  'FM-1_2143'\n]);" not in s: raise SystemExit('V4.1 source allowlist unexpected')
else: raise SystemExit('Expected V4.0 or V4.1 B1 installer')
s,n=re.subn(r"(HASH_ET: ')[0-9a-f]{64}(')",lambda m:m[1]+sha+m[2],s,count=1)
assert n==1
s,n=re.subn(r"(ID_ET: ')[^']+(')",lambda m:m[1]+'FM-1_2143'+m[2],s,count=1)
assert n==1
s,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">)\s*.*?\s*(</script>)',lambda m:m[1]+'\n'+base64.b64encode(fw).decode()+'\n'+m[2],s,count=1,flags=re.S)
assert n==1
assert "  'FM-1_2142',\n  'FM-1_2143'\n]);" in s
assert "ID_ET: 'FM-1_2143'" in s
assert 'B1 V4.0' not in s
html.write_text(s)
print('B1 V4.1: exact FWSC embedded, SHA-256',sha)
