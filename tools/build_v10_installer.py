#!/usr/bin/env python3
"""Rebuild the B1 V1.0 WebInstaller from the new, SHA-pinned FM1 .fwsc.
This tool never connects to or flashes a device.
"""
from pathlib import Path
import base64, hashlib, re
root=Path(__file__).resolve().parent.parent
fw=(root/'build/SikkenWorks-B1-V1.0-BEETLE-OTA.fwsc').read_bytes()
web=root/'B1-WebInstaller.html'
s=web.read_text()
s=s.replace('B1 V0.9','B1 V1.0')
s=s.replace("ID_ET: 'FM-1_2109'", "ID_ET: 'FM-1_2110'")
# Whitelist the new running ID but retain all older BEETLE / B1 updater IDs.
s=s.replace("||identity==='FM-1_2109'", "||identity==='FM-1_2109'||identity==='FM-1_2110'")
s=s.replace("'FM-1_2109'\n]);", "'FM-1_2109','FM-1_2110'\n]);")
s=s.replace(' / FM-1_2109 are supported', ' / FM-1_2109 / FM-1_2110 are supported')
s=s.replace(' / FM-1_2109 and an exact approved package', ' / FM-1_2109 / FM-1_2110 and an exact approved package')
s=s.replace(' / FM-1_2109.', ' / FM-1_2109 / FM-1_2110.')
s=s.replace('HASH_ET: \'aa4ee0ee731caf791c4df7e60c15fb37bf0a490475fa0866a44cbb96930a24de\'',
            "HASH_ET: '"+hashlib.sha256(fw).hexdigest()+"'")
# Future versions: generator is idempotent and works if inherited old hash changes.
if hashlib.sha256(fw).hexdigest() not in s:
    s=re.sub(r"HASH_ET: '[0-9a-f]{64}'", "HASH_ET: '"+hashlib.sha256(fw).hexdigest()+"'", s, count=1)
s,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)[^<]*(</script>)',
            lambda m:m.group(1)+base64.b64encode(fw).decode()+'\n'+m.group(2),s)
assert n==1
assert "ID_ET: 'FM-1_2110'" in s
assert "identity==='FM-1_2110'" in s
assert hashlib.sha256(fw).hexdigest() in s
assert base64.b64decode(re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*([^<]+)',s).group(1))==fw
web.write_text(s)
print('Installer V1.0 sha256',hashlib.sha256(fw).hexdigest())
