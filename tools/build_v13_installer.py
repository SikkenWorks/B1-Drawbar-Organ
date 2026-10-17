#!/usr/bin/env python3
"""Update B1 V1.1 USB-MIDI OTA installer to B1 V1.3, pinning the exact file hash.
This does NOT mean hardware installation/recovery is proven.
"""
from pathlib import Path
import re, hashlib, base64
ROOT=Path(__file__).resolve().parent.parent
fw=(ROOT/'build/SikkenWorks-B1-V1.3-BEETLE-OTA.fwsc').read_bytes()
hash_=hashlib.sha256(fw).hexdigest()
html=ROOT/'B1-WebInstaller.html'
s=html.read_text()
s=s.replace('B1 V1.2','B1 V1.3')
s=s.replace("ID_ET: 'FM-1_2112'", "ID_ET: 'FM-1_2113'")
s,n=re.subn(r"HASH_ET: '[0-9a-f]{64}'","HASH_ET: '"+hash_+"'",s,count=1)
assert n==1
known=['FM-1_2101',*['FM-1_'+str(i) for i in range(2104,2114)]]
for function in ('freedomSourceAllowed','isSupportedEtchIdentity'):
 body='function '+function+'(identity){\n  return ' + '||'.join("identity==='"+x+"'" for x in known)+';\n}'
 s,n=re.subn(r'function '+function+r'\(identity\)\s*\{[\s\S]*?\n\}',body,s,count=1)
 assert n==1,(function,n)
s,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)[^<]*(</script>)',lambda m:m.group(1)+base64.b64encode(fw).decode()+'\n'+m.group(2),s)
assert n==1
s=s.replace('B1 V1.2 HARDWARE-UNTESTED','B1 V1.3 HARDWARE-UNTESTED')
assert "ID_ET: 'FM-1_2113'" in s
assert hash_ in s
html.write_text(s)
print('B1 V1.3 installer: verified embedded hash',hash_)
