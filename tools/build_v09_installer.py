#!/usr/bin/env python3
"""Embed verified B1 V0.9 OTA package; keep Felucca transport and BEETLE restore.

No hardware flashes are performed by this generator.
"""
from pathlib import Path
import base64,hashlib,re
root=Path(__file__).resolve().parent.parent
fw=root/'build/SikkenWorks-B1-V0.9-BEETLE-OTA.fwsc'
web=root/'B1-WebInstaller.html'
s=web.read_text()
assert "ID_ET: 'FM-1_2108'" in s or "ID_ET: 'FM-1_2109'" in s
s=s.replace('B1 V0.8','B1 V0.9')
s=s.replace("ID_ET: 'FM-1_2108'", "ID_ET: 'FM-1_2109'")
s=s.replace("identity==='FM-1_2108';", "identity==='FM-1_2108'||identity==='FM-1_2109';")
s=s.replace("identity==='FM-1_2108'||", "identity==='FM-1_2108'||identity==='FM-1_2109'||") if False else s
s=s.replace("||identity==='FM-1_2108'", "||identity==='FM-1_2108'||identity==='FM-1_2109'")
# Remove duplicate condition from preceding replacement, if present.
s=s.replace("||identity==='FM-1_2109'||identity==='FM-1_2109'", "||identity==='FM-1_2109'")
s=s.replace(' / FM-1_2108', ' / FM-1_2108 / FM-1_2109') if "ID_ET: 'FM-1_2108'" in s else s
s=s.replace("'FM-1_2107','FM-1_2108'\n]);", "'FM-1_2107','FM-1_2108','FM-1_2109'\n]);")
s=s.replace("'FM-1_2108';", "'FM-1_2108'||identity==='FM-1_2109';") if False else s
s=s.replace("HASH_ET: '"+re.search(r"HASH_ET: '([a-f0-9]{64})'",s).group(1)+"'", "HASH_ET: '"+hashlib.sha256(fw.read_bytes()).hexdigest()+"'")
s,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)[^<]*(</script>)', lambda m:m.group(1)+base64.b64encode(fw.read_bytes()).decode()+'\n'+m.group(2), s)
assert n==1
assert "ID_ET: 'FM-1_2109'" in s
assert "identity==='FM-1_2109'" in s
web.write_text(s)
print('Installer pinned to firmware sha256',hashlib.sha256(fw.read_bytes()).hexdigest())
