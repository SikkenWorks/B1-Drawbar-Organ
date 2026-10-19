#!/usr/bin/env python3
"""Pin and embed the target-compiled V3.7 firmware into the B1 WebInstaller.

Starts from the existing audited B1 installer: only identity, version and
embedded image change. No recovery / source whitelist checks are weakened.
"""
from pathlib import Path
import base64, hashlib, re
root=Path(__file__).resolve().parent.parent
html=root/'B1-WebInstaller.html'
firmware=root/'build/SikkenWorks-B1-V3.7-BEETLE-OTA.fwsc'
fw=firmware.read_bytes()
if len(fw)!=610086:raise SystemExit('Wrong firmware package size: '+str(len(fw)))
sha=hashlib.sha256(fw).hexdigest()
s=html.read_text()
# Convert only the installer-owned B1 release branding, never the other
# source firmware identities or the protected header's pinned digest.
s=s.replace('B1 V3.6','B1 V3.7').replace('B1-V3.6','B1-V3.7')
s,n=re.subn(r"(HASH_ET: ')[0-9a-f]{64}(')",lambda m:m[1]+sha+m[2],s,count=1)
assert n==1
s,n=re.subn(r"(ID_ET: ')[^']+(')",lambda m:m[1]+'FM-1_2137'+m[2],s,count=1)
assert n==1
# Keep V3.6 in the known-source whitelist so upgrades from V3.6 work.
assert "'FM-1_2136'" in s and "'FM-1_2137'" in s
s,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">)\s*.*?\s*(</script>)',
           lambda m:m[1]+'\n'+base64.b64encode(fw).decode()+'\n'+m[2],s,count=1,flags=re.S)
assert n==1
# Guard against accidentally embedding an older firmware or permitting unknown ids.
if "ID_ET: 'FM-1_2137'" not in s or "'FM-1_2136'" not in s:
 raise SystemExit('Installer identity failure')
if s.count('B1 V3.6'):raise SystemExit('Stale release label')
html.write_text(s)
print('Embedded V3.7 FWSC',len(fw),sha)
