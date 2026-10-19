#!/usr/bin/env python3
"""Embed exactly the freshly compiled B1 V4.0 AC791N firmware.

Never update the approved boot-header hash, reference loader, target permissions
or installer security checks here. B1 V3.9 remains an allowed upgrade source.
"""
from pathlib import Path
import base64,hashlib,re
root=Path(__file__).resolve().parent.parent
html=root/'B1-WebInstaller.html'
fw=(root/'build/SikkenWorks-B1-V4.0-BEETLE-OTA.fwsc').read_bytes()
if len(fw)!=610086:raise SystemExit('Unexpected .fwsc size')
sha=hashlib.sha256(fw).hexdigest()
s=html.read_text()
if "ID_ET: 'FM-1_2139'" in s:
 if "  'FM-1_2139'\n]);" not in s:raise SystemExit('Unexpected V3.9 approved source list')
 s=s.replace('B1 V3.9','B1 V4.0').replace('B1-V3.9','B1-V4.0')
 s=s.replace("  'FM-1_2139'\n]);", "  'FM-1_2139',\n  'FM-1_2142'\n]);")
 s=s.replace('FM-1_2139. Built-in exact pinned', 'FM-1_2142. Built-in exact pinned')
elif "ID_ET: 'FM-1_2142'" in s:
 if "  'FM-1_2142'\n]);" not in s:raise SystemExit('Expected V4.0 approved source list')
else:raise SystemExit('Expected the V3.9 or V4.0 B1 installer')
s,n=re.subn(r"(HASH_ET: ')[0-9a-f]{64}(')",lambda m:m[1]+sha+m[2],s,count=1)
assert n==1
s,n=re.subn(r"(ID_ET: ')[^']+(')",lambda m:m[1]+'FM-1_2142'+m[2],s,count=1)
assert n==1
s,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">)\s*.*?\s*(</script>)',lambda m:m[1]+'\n'+base64.b64encode(fw).decode()+'\n'+m[2],s,count=1,flags=re.S)
assert n==1
assert "  'FM-1_2139',\n  'FM-1_2142'\n]);" in s
assert "ID_ET: 'FM-1_2142'" in s
assert 'B1 V3.9' not in s
html.write_text(s)
print('B1 V4.0: exact FWSC embedded, SHA-256',sha)
