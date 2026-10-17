#!/usr/bin/env python3
"""Embed exactly the verified B1 V1.7 .fwsc; preserve approved restore path."""
from pathlib import Path
import re,hashlib,base64
ROOT=Path(__file__).resolve().parent.parent
fw=(ROOT/'build/SikkenWorks-B1-V1.7-BEETLE-OTA.fwsc').read_bytes()
sha=hashlib.sha256(fw).hexdigest()
p=ROOT/'B1-WebInstaller.html';s=p.read_text()
s=s.replace('B1 V1.6','B1 V1.7').replace('B1-V1.6','B1-V1.7')
s=s.replace("ID_ET: 'FM-1_2116'", "ID_ET: 'FM-1_2117'")
s,n=re.subn(r"HASH_ET: '[0-9a-f]{64}'", "HASH_ET: '"+sha+"'", s, count=1);assert n==1
# Existing approved firmware-to-firmware routes remain available.
for fn in ('freedomSourceAllowed','isSupportedEtchIdentity'):
    pattern=r'function '+fn+r'\(identity\)\s*\{[\s\S]*?\n\}'
    m=re.search(pattern,s);assert m,fn
    block=m.group(0)
    assert "identity==='FM-1_2116'" in block
    block=block.replace("identity==='FM-1_2116';", "identity==='FM-1_2116'||identity==='FM-1_2117';")
    s=s[:m.start()]+block+s[m.end():]
s,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)[^<]*(</script>)',lambda m:m.group(1)+base64.b64encode(fw).decode()+'\n'+m.group(2),s,count=1);assert n==1
s=s.replace('B1 V1.7 uses 12 named persistent project slots in a two-copy flash journal at 0xE8000–0xE9FFF, separate from BEETLE project sectors (0x97000–0xDFFFF). B1 checks for foreign data and refuses to overwrite it. An explicit SAVE can write only to those two B1 sectors; firmware upgrades separately use the existing OTA staging sectors (0xE0000–0xE4FFF).', 'B1 V1.7 scans 0xE7000–0xEFFFF for two positively verified blank or B1-owned 4KiB sectors for 12 named persistent projects. Any unfamiliar, corrupt, or foreign content is NEVER erased. If two safe sectors cannot be found, saving remains locked. BEETLE projects (0x97000–0xDFFFF) and OTA staging (0xE0000–0xE4FFF) remain separate.')
assert "ID_ET: 'FM-1_2117'" in s and sha in s
p.write_text(s)
print('V1.7 WebInstaller embedded SHA-256',sha)
