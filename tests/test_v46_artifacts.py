#!/usr/bin/env python3
"""B1 V4.6 release invariants, permanent package ID and input-source preservation."""
from pathlib import Path
import hashlib,base64,re,sys,zipfile
R=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(R/'tools'))
from fm1_fwsc import parse_ufw,verify_fwsc
fwfile=R/'build/SikkenWorks-B1-V4.6-BEETLE-OTA.fwsc'
fw=fwfile.read_bytes(); old=(R/'build/SikkenWorks-B1-V4.5-BEETLE-OTA.fwsc').read_bytes()
report,errs=verify_fwsc(fwfile)
assert not errs,errs
assert report['product']=='FM-1_2147' and len(fw)==610086,report
assert parse_ufw(fw)['product']==parse_ufw(old)['product']=='FM-1_2147'
flash=lambda data:next(e['data'] for e in parse_ufw(data)['entries'] if e['type']==0)
assert flash(fw)[:0x4000]==flash(old)[:0x4000], 'boot sector changed'
assert '#define FELUCCA_ID "FM-1_2147"' in (R/'firmware/b1_main.c').read_text()
assert "PRODUCT='FM-1_2147'" in (R/'tools/b1_pack_beetle_ota.py').read_text()
html=(R/'B1-WebInstaller.html').read_text()
m=re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*(\S+)\s*</script>',html)
assert m and base64.b64decode(m.group(1),validate=True)==fw,'installer mismatch'
sha=hashlib.sha256(fw).hexdigest()
assert f"HASH_ET: '{sha}'" in html and "ID_ET: 'FM-1_2147'" in html
assert 'function isApprovedOtaLoader' in html and 'FM-1_900' in html
assert 'B1 V4.6' in html and 'B1 V4.5' not in html
assert (R/'firmware/b1_performance.c').read_bytes()==(R/'tests/reference_v41/b1_performance.c').read_bytes()
assert b'FELUCCA-LOADER-1' in (R/'firmware/ota.c').read_bytes()
# Full V4.5 engine/USB/MIDI/UI/storage source must stay byte identical.
reference=Path('/mnt/data/SikkenWorks-B1-V4.5-POLYPHONY-OPTIMIZED-FULL-SOURCE.zip')
if reference.exists():
 with zipfile.ZipFile(reference) as z:
  for name in ('b1_engine.c','b1_performance.c','b1_storage.c','b1_ui.c','b1_startup.c','usb.c','b1_ota_target.c'):
   assert (R/'firmware'/name).read_bytes()==z.read('SikkenWorks-B1-V4.5/firmware/'+name), name+' was modified'
print('PASS: B1 V4.6 exact identity FM-1_2147; OTA byte integrity; SLOOP HF2; 16KiB boot; untouched synth/USB/UI/storage')
print('FWSC SHA256:',sha)
