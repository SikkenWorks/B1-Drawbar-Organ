#!/usr/bin/env python3
"""Embed the audited B1 V0.5 FWSC into existing, working V0.4 Web MIDI installer.
Do not replace or redesign the OTA transport. All changes are explicit/string-audited.
"""
from pathlib import Path
import re,base64,json
root=Path(__file__).resolve().parent.parent
html_file=root/'B1-WebInstaller.html'
fwsc=root/'build/SikkenWorks-B1-V0.5-BEETLE-OTA.fwsc'
manifest=json.loads((root/'build/RELEASE-MANIFEST.json').read_text())
s=html_file.read_text()
s=s.replace('B1 V0.4','B1 V0.5').replace('B1 V0.4','B1 V0.5')
s=s.replace('SIKKENWORKS B1 V0.4','SIKKENWORKS B1 V0.5').replace('SikkenWorks B1 V0.4','SikkenWorks B1 V0.5')
s=s.replace("HASH_ET: '7e9d9046af36c16d8800b562ddace4632e350574241897cf3d661ac744b1f79b'",f"HASH_ET: '{manifest['fwsc_sha256']}'")
s=s.replace("ID_ET: 'FM-1_2104'","ID_ET: 'FM-1_2105'")
# Version 0.4 remains authorized as an update source (same protected 16 KiB header).
s=s.replace("'FM-1_2100','FM-1_2101','FM-1_2104'","'FM-1_2100','FM-1_2101','FM-1_2104','FM-1_2105'")
s=s.replace("return (identity==='FM-1_2104'||identity==='FM-1_2101');","return (identity==='FM-1_2105'||identity==='FM-1_2104'||identity==='FM-1_2101');")
s=s.replace("return identity==='FM-1_2101'||identity==='FM-1_2104';","return identity==='FM-1_2101'||identity==='FM-1_2104'||identity==='FM-1_2105';")
s=s.replace('BEETLE FM-1_2101 or B1 FM-1_2104','BEETLE FM-1_2101 or B1 FM-1_2104 / FM-1_2105')
s=s.replace('BEETLE FM-1_2101 and B1 FM-1_2104','BEETLE FM-1_2101 and B1 FM-1_2104 / FM-1_2105')
s=s.replace('only BEETLE FM-1_2101 and B1 FM-1_2104','only BEETLE FM-1_2101 and B1 FM-1_2104 / FM-1_2105')
s=s.replace('Only BEETLE FM-1_2101 and B1 FM-1_2104','Only BEETLE FM-1_2101 and B1 FM-1_2104 / FM-1_2105')
s=s.replace("SikkenWorks B1 V0.4 hardware-unverified candidate","SikkenWorks B1 V0.5 hardware-unverified candidate")
s=s.replace('FM-1_2104 are supported in this installer','FM-1_2104 / FM-1_2105 are supported in this installer')
s=s.replace('B1 V0.4 install path enabled','B1 V0.5 install path enabled')
# The base64 is firmware transport data, not a depiction of UI.
base64_data=base64.b64encode(fwsc.read_bytes()).decode('ascii')
pat=re.compile(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)[^<]*(</script>)')
s,n=pat.subn(lambda m:m.group(1)+base64_data+'\n'+m.group(2),s)
assert n==1,'Embedded FWSC script missing'
assert "ID_ET: 'FM-1_2105'" in s and f"HASH_ET: '{manifest['fwsc_sha256']}'" in s
assert "'FM-1_2104'||identity==='FM-1_2105'" in s
assert base64.b64decode(re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*([^<]+)',s).group(1))==fwsc.read_bytes()
html_file.write_text(s)
print('WebInstaller updated',html_file, 'firmware payload bytes',fwsc.stat().st_size)
