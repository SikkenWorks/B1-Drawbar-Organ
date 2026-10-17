#!/usr/bin/env python3
"""B1 V1.5 exact embedded candidate; retains source identities and recovery controls."""
from pathlib import Path
import re, hashlib, base64
ROOT=Path(__file__).resolve().parent.parent
fw=(ROOT/'build/SikkenWorks-B1-V1.5-BEETLE-OTA.fwsc').read_bytes()
sha=hashlib.sha256(fw).hexdigest()
html=ROOT/'B1-WebInstaller.html';s=html.read_text()
s=s.replace('B1 V1.4','B1 V1.5').replace('B1-V1.4','B1-V1.5')
s=s.replace("ID_ET: 'FM-1_2114'", "ID_ET: 'FM-1_2115'")
s,n=re.subn(r"HASH_ET: '[0-9a-f]{64}'","HASH_ET: '"+sha+"'",s,count=1)
assert n==1
known=['FM-1_2101',*['FM-1_'+str(i) for i in range(2104,2116)]]
for fn in ('freedomSourceAllowed','isSupportedEtchIdentity'):
 body='function '+fn+'(identity){\n  return '+'||'.join("identity==='"+x+"'" for x in known)+';\n}'
 s,n=re.subn(r'function '+fn+r'\(identity\)\s*\{[\s\S]*?\n\}',body,s,count=1)
 assert n==1,(fn,n)
s,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)[^<]*(</script>)',lambda m:m.group(1)+base64.b64encode(fw).decode()+'\n'+m.group(2),s)
assert n==1
assert 'FM-1_2115' in s and sha in s
html.write_text(s)
print('Embedded verified V1.5 firmware',sha)
