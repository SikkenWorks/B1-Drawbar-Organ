"""Compare V5.9 PCM and complete engine state to the pinned V5.8 source."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as tmp:
 for test in ('bench_v59_poly','native_v59_poly_edges'):
  results=[]
  for version,src in [('old','tests/reference_v58/b1_engine.c'),('new','firmware/b1_engine.c')]:
   exe=Path(tmp)/version
   subprocess.run(['cc','-O2','-std=c11','-Ifirmware','tests/'+test+'.c',src,'-o',exe],cwd=root,check=True)
   lines=subprocess.check_output([exe],text=True).splitlines()
   results.append([s.rsplit(' ',1)[0] for s in lines] if test.startswith('bench') else lines)
  assert results[0]==results[1],test
  print('PASS',test,len(lines),'V5.8/V5.9 PCM and engine-state scenarios')
