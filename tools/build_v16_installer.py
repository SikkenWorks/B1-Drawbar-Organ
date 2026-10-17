#!/usr/bin/env python3
"""B1 V1.6 exact embedded candidate; retains source identities and recovery controls."""
from pathlib import Path
import re, hashlib, base64
ROOT=Path(__file__).resolve().parent.parent
fw=(ROOT/'build/SikkenWorks-B1-V1.6-BEETLE-OTA.fwsc').read_bytes()
sha=hashlib.sha256(fw).hexdigest()
html=ROOT/'B1-WebInstaller.html';s=html.read_text()
s=s.replace('B1 V1.5','B1 V1.6').replace('B1-V1.5','B1-V1.6')
s=s.replace("ID_ET: 'FM-1_2115'", "ID_ET: 'FM-1_2116'")
s,n=re.subn(r"HASH_ET: '[0-9a-f]{64}'","HASH_ET: '"+sha+"'",s,count=1)
assert n==1
known=['FM-1_2101',*['FM-1_'+str(i) for i in range(2104,2117)]]
for fn in ('freedomSourceAllowed','isSupportedEtchIdentity'):
 body='function '+fn+'(identity){\n  return '+'||'.join("identity==='"+x+"'" for x in known)+';\n}'
 s,n=re.subn(r'function '+fn+r'\(identity\)\s*\{[\s\S]*?\n\}',body,s,count=1)
 assert n==1,(fn,n)
s,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)[^<]*(</script>)',lambda m:m.group(1)+base64.b64encode(fw).decode()+'\n'+m.group(2),s)
assert n==1
assert 'FM-1_2116' in s and sha in s
# The web installer must describe the new persistent storage policy truthfully.
s=s.replace('B1 V1.6 does not initialize, erase, or overwrite BEETLE project sectors. Save/Load uses four temporary RAM slots only; content is lost when you power off. The ONLY flash writes implemented by the B1 application are confined to the dedicated FM1 OTA staging sectors (0xE0000–0xE4FFF) during an explicitly requested firmware upgrade.', 'B1 V1.6 uses 12 named persistent project slots in a two-copy flash journal at 0xE8000–0xE9FFF, separate from BEETLE project sectors (0x97000–0xDFFFF). B1 checks for foreign data and refuses to overwrite it. An explicit SAVE can write only to those two B1 sectors; firmware upgrades separately use the existing OTA staging sectors (0xE0000–0xE4FFF).')
s=s.replace('B1 V1.6 / ORGAN','B1 V1.6 / ROCK ORGAN')
s=s.replace('Native B1 rock-organ build. BEETLE project storage is not written. OTA staging is enabled for firmware changes.', 'Native B1 rock-organ build. 12 named projects persist in B1-only flash sectors. BEETLE project storage is not written. OTA staging remains supported.')
assert '12 named persistent project slots' in s
html.write_text(s)
print('Embedded verified V1.6 firmware',sha)
