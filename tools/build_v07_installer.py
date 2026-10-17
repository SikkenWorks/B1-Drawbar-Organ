#!/usr/bin/env python3
"""Embed exactly the newly built B1 V0.7 image into the existing verified OTA host flow.
Does NOT prove hardware boot or return-to-stock. No flashing is done here.
"""
from pathlib import Path
import re,base64,hashlib
root=Path(__file__).resolve().parent.parent
html=root/'B1-WebInstaller.html'
fw=root/'build/SikkenWorks-B1-V0.7-BEETLE-OTA.fwsc'
s=html.read_text()
if "ID_ET: 'FM-1_2106'" not in s:raise SystemExit('Expected V0.6 installer; refusing to modify unknown template')
s=s.replace('B1 V0.6','B1 V0.7')
s=s.replace("ID_ET: 'FM-1_2106'", "ID_ET: 'FM-1_2107'")
s=re.sub(r"HASH_ET: '[a-f0-9]{64}'", "HASH_ET: '"+hashlib.sha256(fw.read_bytes()).hexdigest()+"'",s,count=1)
# Recognize B1 V0.6 as a prior source, and V0.7 as both target and source for restoring BEETLE.
s=s.replace("'FM-1_2105','FM-1_2106'", "'FM-1_2105','FM-1_2106','FM-1_2107'")
s=s.replace("identity==='FM-1_2106'", "identity==='FM-1_2106'||identity==='FM-1_2107'")
s=s.replace("||identity==='FM-1_2106';", "||identity==='FM-1_2106'||identity==='FM-1_2107';")
# Update human-readable source-compatibility descriptions, do not change BEETLE identity.
s=s.replace('FM-1_2104 / FM-1_2105 / FM-1_2106', 'FM-1_2104 / FM-1_2105 / FM-1_2106 / FM-1_2107')
s=s.replace('FM-1_2104 / FM-1_2105 / FM-1_2106 / FM-1_2105', 'FM-1_2104 / FM-1_2105 / FM-1_2106 / FM-1_2107')
s=s.replace('BEETLE FM-1_2101 or B1 FM-1_2104 / FM-1_2105 / FM-1_2106', 'BEETLE FM-1_2101 or B1 FM-1_2104 / FM-1_2105 / FM-1_2106 / FM-1_2107')
payload=base64.b64encode(fw.read_bytes()).decode()
pat=re.compile(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)[^<]*(</script>)')
s,count=pat.subn(lambda m:m.group(1)+payload+'\n'+m.group(2),s)
assert count==1
assert "ID_ET: 'FM-1_2107'" in s and "identity==='FM-1_2107'" in s
html.write_text(s)
print('V0.7 embedded',len(fw.read_bytes()),'bytes, SHA-256:',hashlib.sha256(fw.read_bytes()).hexdigest())
