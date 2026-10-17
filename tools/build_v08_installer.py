#!/usr/bin/env python3
"""Regenerate V0.8 webinstaller from the audited B1 V0.7 template in this source tree.
Uses same exact firmware and preserves verified BEETLE return package. No flashing.
"""
from pathlib import Path
import hashlib,base64,re
root=Path(__file__).resolve().parent.parent
fw=root/'build/SikkenWorks-B1-V0.8-BEETLE-OTA.fwsc'
web=root/'B1-WebInstaller.html'
s=web.read_text()
assert "ID_ET: 'FM-1_2108'" in s
s=re.sub(r"HASH_ET: '[a-f0-9]{64}'",lambda m:"HASH_ET: '"+hashlib.sha256(fw.read_bytes()).hexdigest()+"'",s,count=1)
s,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)[^<]*(</script>)',lambda m:m.group(1)+base64.b64encode(fw.read_bytes()).decode()+'\n'+m.group(2),s)
assert n==1
web.write_text(s)
print('B1 V0.8 installer firmware embedded and pinned to SHA-256.')
