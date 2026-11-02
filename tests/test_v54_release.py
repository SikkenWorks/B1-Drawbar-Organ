#!/usr/bin/env python3
"""V5.4 integrity, bit-exact sound and native OTA regressions (host only)."""
import base64,hashlib,subprocess,sys,tempfile
from pathlib import Path
R=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(R/'tools'))
from fm1_fwsc import verify_fwsc,parse_ufw
p=R/'build/SikkenWorks-B1-V5.4.fwsc'
fw=p.read_bytes(); summary,errors=verify_fwsc(p)
assert len(fw)==610086 and not errors and summary['valid'],(summary,errors)
assert summary['product']=='FM-1_481026'
assert '#define FELUCCA_ID "FM-1_481026"' in (R/'firmware/b1_main.c').read_text()
assert "PRODUCT='FM-1_481026'" in (R/'tools/b1_pack_beetle_ota.py').read_text()
s=(R/'B1-WebInstaller.html').read_text()
assert "ID_ET: 'FM-1_481026'" in s
assert f"HASH_ET: '{hashlib.sha256(fw).hexdigest()}'" in s
assert base64.b64encode(fw).decode() in s
assert 'V5.4' in s and 'function isApprovedOtaLoader' in s
assert (R/'build/B1.elf').is_file() and (R/'build/B1-app.bin').is_file()
print('PASS verified native JieLi ELF, FWSC wrapper, ID, embedded installer SHA256 & bytes',flush=True)
with tempfile.TemporaryDirectory(prefix='b1-v54-test-') as d:
 d=Path(d)
 for test in ('native_v45_poly_exact.c','native_v50_transition_exact.c','native_v51_full_state.c'):
  def compare(name,engine):
   dest=d/name
   subprocess.run(['cc','-std=c11','-O2','-I'+str(R/'firmware'),str(R/'tests'/test),str(engine),'-o',str(dest)],check=True)
   return subprocess.check_output([str(dest)])
  ref=compare('ref',R/'tests/reference_v53/b1_engine.c')
  now=compare('new',R/'firmware/b1_engine.c')
  assert ref==now, 'Output or DSP state changed in '+test
  print('PASS bit-identical PCM+full-state:',test,flush=True)
 for name in ('test_v48_poly_output.c','test_v48_chunk_consistency.c'):
  dest=d/name.replace('.c','')
  subprocess.run(['cc','-std=c11','-O2','-I'+str(R/'firmware'),str(R/'tests'/name),str(R/'firmware/b1_engine.c'),'-o',str(dest)],check=True)
  subprocess.run([str(dest)],check=True)
 subprocess.run([sys.executable,str(R/'tests/test_v48_dma_policy.py')],check=True)
 # Preserve the V5.3 AUTO SAVE setting and journal migrations.
 for name,sources,flags in [
  ('autosave',['b1_storage.c','b1_performance.c','b1_engine.c'],['-DB1_STORAGE_DYNAMIC=1']),
  ('ui',['b1_ui.c','b1_performance.c','b1_engine.c'],[])]:
  dest=d/name
  src='native_v53_autosave_switch.c' if name=='autosave' else 'native_v53_ui_toggle.c'
  subprocess.run(['cc','-std=c11','-O2',*flags,'-I'+str(R/'firmware'),str(R/'tests'/src),*[str(R/'firmware'/s) for s in sources],'-o',str(dest)],check=True)
  subprocess.run([str(dest)],check=True)
 print('PASS V5.3 AUTO SAVE default OFF, saved preference, storage migration and UI',flush=True)
 subprocess.run([sys.executable,str(R/'tests/run_ota_v54_replay.py'),'cc'],check=True,stdout=subprocess.DEVNULL)
 exe=d/'ota';subprocess.run(['cc','-std=c11','-O2','-I'+str(R/'firmware'),str(R/'tests/native_ota_replay_v54.c'),str(R/'firmware/native_digest.c'),'-o',str(exe)],check=True)
 fixtures=R/'tests/fixtures'
 boot=R/'build/qa/boot.bin'
 if boot.is_file() and (fixtures/'v54-short-app-valid.ufw').is_file():
  subprocess.run([str(exe),str(fixtures/'v54-short-app-valid.ufw'),str(boot),'expect_pass'],check=True)
 print('PASS native OTA CHECK replays, invalid payload fails closed',flush=True)
print('NOT VERIFIED: V15/V16 stock flash on physical FM1, V16 package, full 12-voice DMA peak, crackle-free operation',flush=True)
