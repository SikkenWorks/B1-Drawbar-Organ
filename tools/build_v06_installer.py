#!/usr/bin/env python3
"""Embed the verified V0.6 target image, preserving V0.5 USB MIDI updater code.
No flash or hardware access is performed by this script.
"""
from pathlib import Path
import re,base64,hashlib
root=Path(__file__).resolve().parent.parent
html=root/'B1-WebInstaller.html'
fw=root/'build/SikkenWorks-B1-V0.6-BEETLE-OTA.fwsc'
s=html.read_text()
s=s.replace('B1 V0.5','B1 V0.6')
# Distinguish running V0.5 from the *new* V0.6 package target.
s=s.replace("ID_ET: 'FM-1_2105'", "ID_ET: 'FM-1_2106'")
s=re.sub(r"HASH_ET: '[a-f0-9]{64}'", "HASH_ET: '"+hashlib.sha256(fw.read_bytes()).hexdigest()+"'",s,count=1)
s=s.replace("identity==='FM-1_2105'", "identity==='FM-1_2105'||identity==='FM-1_2106'")
s=s.replace("'FM-1_2104','FM-1_2105'", "'FM-1_2104','FM-1_2105','FM-1_2106'")
s=s.replace('FM-1_2104 / FM-1_2105', 'FM-1_2104 / FM-1_2105 / FM-1_2106')
s=s.replace('FM-1_2105 / FM-1_2105', 'FM-1_2105 / FM-1_2106')
payload=base64.b64encode(fw.read_bytes()).decode()
pat=re.compile(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)[^<]*(</script>)')
s,count=pat.subn(lambda m:m.group(1)+payload+'\n'+m.group(2),s)
assert count==1
assert "ID_ET: 'FM-1_2106'" in s and "identity==='FM-1_2105'||identity==='FM-1_2106'" in s
html.write_text(s)
print('Embedded',len(fw.read_bytes()),'bytes with SHA-256',hashlib.sha256(fw.read_bytes()).hexdigest())
