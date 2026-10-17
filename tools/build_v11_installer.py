#!/usr/bin/env python3
"""Embed byte-identical V1.1 .fwsc in the existing, hash-pinned FM1 WebInstaller."""
from pathlib import Path
import hashlib,base64,re
root=Path(__file__).resolve().parent.parent
fw=(root/'build/SikkenWorks-B1-V1.1-BEETLE-OTA.fwsc').read_bytes()
s=(root/'B1-WebInstaller.html').read_text()
s=s.replace('B1 V1.0','B1 V1.1')
s=s.replace('if(VERIFIED_RUNNING_FIRMWARE.has(identity))', 'if(VERIFIED_RUNNING_FIRMWARE.has(identity)||isSupportedEtchIdentity(identity))')
s=s.replace("ID_ET: 'FM-1_2110'", "ID_ET: 'FM-1_2111'")
s=re.sub(r"HASH_ET: '[0-9a-f]{64}'", "HASH_ET: '"+hashlib.sha256(fw).hexdigest()+"'", s,count=1)
# Tight source allowlists: carry forward known B1 identities plus V1.1, never
# treat arbitrary FM1_* as safe. An exact physical/header test is still needed.
known=['FM-1_2101','FM-1_2104','FM-1_2105','FM-1_2106','FM-1_2107','FM-1_2108','FM-1_2109','FM-1_2110','FM-1_2111']
for function in ('freedomSourceAllowed','isSupportedEtchIdentity'):
 new='function '+function+'(identity){\n  return '+"||".join("identity==='"+item+"'" for item in known)+';\n}'
 s,n=re.subn(r'function '+function+r'\(identity\)\s*\{[\s\S]*?\n\}',new,s,count=1)
 assert n==1,(function,n)
# Enhance user-visible failure messages for newly known B1.
s=s.replace('FM-1_2109 / FM-1_2110', 'FM-1_2109 / FM-1_2110 / FM-1_2111')
s=s.replace(' / FM-1_2110 and an exact approved package',' / FM-1_2110 / FM-1_2111 and an exact approved package')
s,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)[^<]*(</script>)',lambda m:m.group(1)+base64.b64encode(fw).decode()+'\n'+m.group(2),s)
assert n==1
assert "ID_ET: 'FM-1_2111'" in s and "FM-1_2110'" in s
assert hashlib.sha256(fw).hexdigest() in s
assert base64.b64decode(re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*([^<]+)',s).group(1))==fw
(root/'B1-WebInstaller.html').write_text(s)
print('WebInstaller embedded V1.1, SHA256 '+hashlib.sha256(fw).hexdigest())
