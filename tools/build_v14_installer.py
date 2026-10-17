#!/usr/bin/env python3
"""Embed the exact B1 V1.4 .fwsc in the existing USB MIDI installer.
Offline verification only. No claim of hardware-proven OTA/recovery.
"""
from pathlib import Path
import re, hashlib, base64
ROOT=Path(__file__).resolve().parent.parent
fw=(ROOT/'build/SikkenWorks-B1-V1.4-BEETLE-OTA.fwsc').read_bytes()
sha=hashlib.sha256(fw).hexdigest()
html=ROOT/'B1-WebInstaller.html';s=html.read_text()
s=s.replace('B1 V1.3','B1 V1.4').replace('B1-V1.3','B1-V1.4')
s=s.replace("ID_ET: 'FM-1_2113'", "ID_ET: 'FM-1_2114'")
s,n=re.subn(r"HASH_ET: '[0-9a-f]{64}'","HASH_ET: '"+sha+"'",s,count=1)
assert n==1,'hash field missing'
known=['FM-1_2101',*['FM-1_'+str(i) for i in range(2104,2115)]]
for fn in ('freedomSourceAllowed','isSupportedEtchIdentity'):
 body='function '+fn+'(identity){\n  return '+'||'.join("identity==='"+x+"'" for x in known)+';\n}'
 s,n=re.subn(r'function '+fn+r'\(identity\)\s*\{[\s\S]*?\n\}',body,s,count=1)
 assert n==1,(fn,n)
s,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)[^<]*(</script>)',lambda m:m.group(1)+base64.b64encode(fw).decode()+'\n'+m.group(2),s)
assert n==1,'embedded fwsc missing'
assert "FM-1_2114" in s and sha in s
html.write_text(s)
print('Installer V1.4 embedded 1:1 SHA',sha)
