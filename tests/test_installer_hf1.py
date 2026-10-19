from pathlib import Path
import hashlib,base64,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
for version in ("4.2","4.3"):
    h=(root / ("B1-WebInstaller.html" if version=="4.3" else "SikkenWorks-B1-V4.2-Installer-HF1.html")).read_text()
    m=re.search(r'<script id="firmware-embedded" type="application/octet-stream">(.*?)</script>',h,re.S)
    assert m,version
    fw=base64.b64decode(m.group(1))
    assert fw==(root/f"SikkenWorks-B1-V{version}-BEETLE-OTA.fwsc").read_bytes(),version
    assert 'legacy-felucca-source' in h and 'legacySourceOptIn()' in h
    assert 'ota-FM-1_015' in h
    assert "dev=await this.waitFor(id=>id.loader && id.text==='ota-FM-1_015'" in h
    print("PASS", version,"firmware SHA-256",hashlib.sha256(fw).hexdigest())
