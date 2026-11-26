#!/usr/bin/env python3
"""B1 V5.9 release gates. NO DEVICE IO. Run after build_v59_release.py."""
from pathlib import Path
import base64, hashlib, re, subprocess, sys, tempfile
root=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(root/'tools'))
from fm1_fwsc import verify_fwsc, parse_ufw
fw=root/'build/SikkenWorks-B1-V5.9.fwsc'
assert fw.is_file(), 'build the target first'
raw=fw.read_bytes(); rpt,err=verify_fwsc(fw)
assert not err and rpt['valid'] and rpt['product']=='FM-1_481026',(rpt,err)
assert len(raw)==610086
# Fixed, reference V5.2 source digests make this test ZIP-independent.
reference_dsp={
    'b1_engine.c':'b545161af2b7e8227da99a2c0ca1ef1210816ed01da04cc857545291ba329440',
    'b1_performance.c':'0506b4ffd55a880b9be25338089f6ca3330051b397fb014cee6c82504db1de17',
}
for key,expected in reference_dsp.items():
    assert hashlib.sha256(((root/'tests/reference_v58' if key=='b1_engine.c' else root/'firmware')/key).read_bytes()).hexdigest()==expected, key+' changed without PCM audit'
assert '#define FELUCCA_ID "FM-1_481026"' in (root/'firmware/b1_main.c').read_text()
html=(root/'B1-WebInstaller.html').read_text();sha=hashlib.sha256(raw).hexdigest()
assert f"HASH_ET: '{sha}'" in html
assert 'ID_ET: \'FM-1_481026\'' in html
assert base64.b64encode(raw).decode() in html
assert 'B1 V5.9' in html and "device.id.text==='FM-1_016'" in html
assert 'b1_ota_known_installed_boot' in (root/'firmware/ota.c').read_text()
assert 'b1_ota_official_v15_writer' in (root/'firmware/ota.c').read_text()
assert 'b1_flash_supported_id(b1_flash_jedec)' in (root/'firmware/b1_main.c').read_text()
print('PASS: native AC791N package, USB identity, embedded exact FWSC/SHA256, V16 warning')
print('PASS: pinned V5.8 reference engine and unchanged performance scheduler')
with tempfile.TemporaryDirectory() as d:
    subprocess.run(['cc','-O2','-std=c11','-Ifirmware','tests/test_v57_flash_compat.c','-o',str(Path(d)/'flash')],cwd=root,check=True)
    subprocess.run([str(Path(d)/'flash')],check=True,cwd=root)
    subprocess.run(['cc','-O2','-std=c11','-Ifirmware','tests/test_v52_arithmetic.c','-o',str(Path(d)/'exact')],cwd=root,check=True)
    subprocess.run([str(Path(d)/'exact')],check=True,cwd=root)
subprocess.run([sys.executable,'tests/test_v58_runtime.py'],cwd=root,check=True)
subprocess.run([sys.executable,'tests/test_v59_poly.py'],cwd=root,check=True)
subprocess.run([sys.executable,'tests/test_v52_lossless.py'],cwd=root,check=True)
subprocess.run([sys.executable,'tests/test_v48_dma_policy.py'],cwd=root,check=True)
subprocess.run([sys.executable,'tests/run_ota_v59_replay.py'],cwd=root,check=True)
# Check the actual executable JS independently from the huge embedded binary.
try:
    js=html.split('<script>',1)[1].split('</script>',1)[0]
    with tempfile.TemporaryDirectory() as d:
        code=Path(d)/'installer.js';code.write_text(js)
        subprocess.run(['node','--check',str(code)],check=True)
    print('PASS: browser installer JavaScript syntax')
except FileNotFoundError:
    print('SKIP: node not installed for installer JS syntax check')
print('PASS: B1 V5.9 offline release validation. Physical FM1 end-to-end NOT TESTED.')
