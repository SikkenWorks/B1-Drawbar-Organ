#!/usr/bin/env python3
"""Create B1 V4.2 installer from exact freshly built FM-1_2144 firmware."""
from pathlib import Path
import base64,hashlib,re
root=Path(__file__).resolve().parent.parent
src=root/'B1-WebInstaller.html'
fw=(root/'build/SikkenWorks-B1-V4.2-BEETLE-OTA.fwsc').read_bytes()
if len(fw)!=610086:raise SystemExit('Wrong V4.2 .fwsc length')
sha=hashlib.sha256(fw).hexdigest()
s=src.read_text()
if "ID_ET: 'FM-1_2143'" in s:
 if "  'FM-1_2143'\n]);" not in s:raise SystemExit('V4.1 allowlist unexpected')
 s=s.replace('B1 V4.1','B1 V4.2').replace('B1-V4.1','B1-V4.2')
 s=s.replace("  'FM-1_2143'\n]);", "  'FM-1_2143',\n  'FM-1_2144'\n]);")
elif "ID_ET: 'FM-1_2144'" in s:
 if "  'FM-1_2144'\n]);" not in s:raise SystemExit('V4.2 allowlist missing')
else:raise SystemExit('Expected V4.1/V4.2 base installer')
s,n=re.subn(r"(HASH_ET: ')[0-9a-f]{64}(')",lambda m:m[1]+sha+m[2],s,count=1)
assert n==1
s,n=re.subn(r"(ID_ET: ')[^']+(')",lambda m:m[1]+'FM-1_2144'+m[2],s,count=1)
assert n==1
s,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">)\s*.*?\s*(</script>)',lambda m:m[1]+'\n'+base64.b64encode(fw).decode()+'\n'+m[2],s,count=1,flags=re.S)
assert n==1 and "  'FM-1_2143',\n  'FM-1_2144'\n]);" in s
assert "ID_ET: 'FM-1_2144'" in s and "HASH_ET: '"+sha+"'" in s
src.write_text(s)
print('V4.2 WebInstaller: source firmware matches embedded, SHA-256',sha)
