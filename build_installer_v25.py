#!/usr/bin/env python3
"""Verify V2.5 SHA and embedded app bytes without disabling checks."""
from pathlib import Path
import re,base64,hashlib
r=Path(__file__).resolve().parent
f=(r/'build/SikkenWorks-B1-V2.5-BEETLE-OTA.fwsc').read_bytes()
s=(r/'B1-WebInstaller.html').read_text()
h=hashlib.sha256(f).hexdigest()
assert re.search(r"HASH_ET: '"+h+"'",s)
assert "ID_ET: 'FM-1_2125'" in s
m=re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*([^<]+)\s*</script>',s)
assert m and base64.b64decode(m.group(1))==f
print('B1 V2.5 installer is SHA-pinned to the current exact native .fwsc:',h)
