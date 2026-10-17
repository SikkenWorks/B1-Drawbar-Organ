#!/usr/bin/env python3
"""Bind V2.4 OTA hash and bytes to the existing safety-checked installer."""
from pathlib import Path
import hashlib,base64,re
root=Path(__file__).resolve().parent
fw=(root/'build/SikkenWorks-B1-V2.4-BEETLE-OTA.fwsc').read_bytes()
s=(root/'B1-WebInstaller.html').read_text()
assert "ID_ET: 'FM-1_2123'" in s
assert "V2.3" in s
s=s.replace('V2.3','V2.4').replace('v2.3','v2.4')
s=s.replace("ID_ET: 'FM-1_2123'","ID_ET: 'FM-1_2124'")
sha=hashlib.sha256(fw).hexdigest()
s,count=re.subn(r"HASH_ET: '[0-9a-f]{64}'",f"HASH_ET: '{sha}'",s,count=1); assert count==1
s,count=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)[^<]*(</script>)',lambda m:m.group(1)+base64.b64encode(fw).decode()+'\n'+m.group(2),s,count=1);assert count==1
# Explicitly allow current B1 as an install and firmware-return source, retain historical identities.
for fn in ('freedomSourceAllowed','isSupportedEtchIdentity'):
 m=re.search(r'function '+fn+r'\(identity\)\s*\{[\s\S]*?\n\}',s);assert m,fn
 t=m.group(0);assert "identity==='FM-1_2123'" in t and "identity==='FM-1_2124'" not in t
 t=t.replace("identity==='FM-1_2123';", "identity==='FM-1_2123'||identity==='FM-1_2124';")
 assert "identity==='FM-1_2124'" in t
 s=s[:m.start()]+t+s[m.end():]
assert sha in s and "ID_ET: 'FM-1_2124'" in s
(root/'B1-WebInstaller.html').write_text(s)
print('V2.4 installer embedded OTA matches',sha)
