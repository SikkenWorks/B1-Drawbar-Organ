#!/usr/bin/env python3
"""Offline B1 V3.8 build/installer integrity gates; never flashes hardware."""
from pathlib import Path
import hashlib,base64,sys,re
sys.path.insert(0,str(Path(__file__).resolve().parent.parent/'tools'))
from fm1_fwsc import verify_fwsc,parse_ufw
root=Path(__file__).resolve().parent.parent
fw=(root/'build/SikkenWorks-B1-V3.8-BEETLE-OTA.fwsc').read_bytes()
ref=(root/'build/BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc').read_bytes()
html=(root/'B1-WebInstaller.html').read_text()
assert len(fw)==610086
report,errors=verify_fwsc(root/'build/SikkenWorks-B1-V3.8-BEETLE-OTA.fwsc')
assert not errors,errors
assert report['product']=='FM-1_2138'
assert report['flash']['app_crc_ok']
assert f"HASH_ET: '{hashlib.sha256(fw).hexdigest()}'" in html
assert "ID_ET: 'FM-1_2138'" in html
embedded=re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*([^<]+)\s*</script>',html)
assert embedded and base64.b64decode(embedded.group(1).strip())==fw
assert "'FM-1_2138'" in html
assert "'FM-1_2136'" in html
assert "'FM-1_2137'" in html
assert 'isSupportedEtchIdentity' in html and 'KNOWN_HEAD_HASH' in html
assert 'B1 V3.8' in html
oflash=next(x['data'] for x in parse_ufw(ref)['entries'] if x['name']=='flash.bin')
nflash=next(x['data'] for x in parse_ufw(fw)['entries'] if x['name']=='flash.bin')
assert nflash[:0x4000]==oflash[:0x4000]
assert hashlib.sha256(fw).digest()!=hashlib.sha256(ref).digest()
print('PASS: genuine V3.8 package, target identity, protected boot region, embedded JS hash + binary')
