#!/usr/bin/env python3
"""Package audited V3.4 artifacts. Never flash hardware from this script."""
from pathlib import Path
import hashlib,io,zipfile,json
from PIL import Image
r=Path(__file__).resolve().parent.parent
out=Path('/mnt/data')
records=json.loads((r/'ui-captures/CAPTURE-MANIFEST.json').read_text())
assert len(records)==67
pngs=[]
for rec in records:
 p=r/'ui-captures'/rec['png']
 with Image.open(p) as im: assert im.size==(240,240)
 pngs.append(p)
for size in (240,960,1920):
 zipname=out/f'SikkenWorks-B1-V3.4-ALL-67-UI-PNG-{size}x{size}.zip'
 with zipfile.ZipFile(zipname,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=8) as z:
  for p in pngs:
   if size==240:z.write(p,p.name)
   else:
    with Image.open(p) as im:
     large=im.resize((size,size),Image.Resampling.NEAREST)
     b=io.BytesIO();large.save(b,format='PNG',optimize=True)
    z.writestr(p.name,b.getvalue())
 with zipfile.ZipFile(zipname) as z:
  assert z.testzip() is None and len(z.namelist())==67
  for n in z.namelist():
   with Image.open(io.BytesIO(z.read(n))) as im:assert im.size==(size,size)
 print(zipname.name,zipname.stat().st_size)

files=[]
for d in ('firmware','tools','tests'):
 for p in (r/d).rglob('*'):
  if p.is_file() and '__pycache__' not in p.parts and p.suffix!='.pyc':files.append(p)
for name in ('B1-WebInstaller.html','B1.gif','README.md','CHANGELOG.md','CREDITS.md','LICENSE','RECOVERY-FIRST.md','TEST-RESULTS-V3.4.md'):
 files.append(r/name)
for name in ('B1.elf','B1-app.bin','SikkenWorks-B1-V3.4-BEETLE-OTA.fwsc','BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc'):
 files.append(r/'build'/name)
files+=pngs
for name in ('B1-V3.4-UI-CONTACT-SHEET.png','B1-V3.4-STARTUP-ANIMATION.gif','CAPTURE-MANIFEST.json'):
 files.append(r/'ui-captures'/name)
for p in (r/'audio-reference').glob('*.wav'):files.append(p)
assert all(p.is_file() for p in files)
output=out/'SikkenWorks-B1-V3.4-REALTIME-OPTIMIZED-FULL-SOURCE.zip'
with zipfile.ZipFile(output,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=7) as z:
 for p in sorted(set(files)):
  z.write(p,arcname='SikkenWorks-B1-V3.4/'+p.relative_to(r).as_posix())
with zipfile.ZipFile(output) as z:
 assert z.testzip() is None and len(z.namelist())==len(set(files))
 assert len(z.read('SikkenWorks-B1-V3.4/README.md'))>7000
 assert b'B1.gif' in z.read('SikkenWorks-B1-V3.4/README.md')
 print('release',output,output.stat().st_size,'files',len(z.namelist()))
fw=r/'build/SikkenWorks-B1-V3.4-BEETLE-OTA.fwsc'
print('firmware SHA-256',hashlib.sha256(fw.read_bytes()).hexdigest())
