from pathlib import Path
import re,base64,hashlib,sys
root=Path(__file__).resolve().parent.parent
fw=(root/'build/SikkenWorks-B1-V4.4-BEETLE-OTA.fwsc').read_bytes()
old=(root/'build/SikkenWorks-B1-V4.3-BEETLE-OTA.fwsc').read_bytes()
if len(fw)!=610086:raise SystemExit('V4.4 wrong bytes')
s=(root/'B1-WebInstaller.html').read_text()
m=re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*(\S+)\s*</script>',s)
if not m or base64.b64decode(m.group(1))!=fw:raise SystemExit('Installer is NOT V4.4 binary')
sha=hashlib.sha256(fw).hexdigest()
if "ID_ET: 'FM-1_2146'" not in s or f"HASH_ET: '{sha}'" not in s or "'FM-1_2146'" not in s:raise SystemExit('Version/SHA missing')
if 'function isApprovedOtaLoader' not in s or 'FM-1_900' not in s:raise SystemExit('HF2 SLOOP support absent')
sys.path.insert(0,str(root/'tools'))
from fm1_fwsc import parse_ufw, verify_fwsc
from tempfile import NamedTemporaryFile
report,errors=verify_fwsc(root/'build/SikkenWorks-B1-V4.4-BEETLE-OTA.fwsc')
if errors or report['product']!='FM-1_2146':raise SystemExit('Invalid FWSC: '+str(errors))
def flash(pkg):
 return next(e['data'] for e in parse_ufw(pkg)['entries'] if e['type']==0)
if flash(fw)[:0x4000]!=flash(old)[:0x4000]:raise SystemExit('Protected boot changed!')
if (root/'firmware/b1_engine.c').read_bytes()!=(root/'tests/reference_v41/b1_engine.c').read_bytes():raise SystemExit('Audio engine modified')
if (root/'firmware/b1_performance.c').read_bytes()!=(root/'tests/reference_v41/b1_performance.c').read_bytes():raise SystemExit('Performance engine modified')
print('PASS V4.4: FM-1_2146 | FWSC/installer bytes + SHA | HF2 | preserved boot & audio source',sha)
