"""Paired, CPU-pinned Linux host benchmark. Run separately from release tests."""
from pathlib import Path
import csv,os,platform,statistics,subprocess,tempfile,sys
root=Path(__file__).resolve().parents[1]
os.sched_setaffinity(0,{min(os.sched_getaffinity(0))})
with tempfile.TemporaryDirectory() as tmp:
 exes={}
 for name,src in [('V5.8','tests/reference_v58/b1_engine.c'),('V5.9','firmware/b1_engine.c')]:
  exe=Path(tmp)/name
  subprocess.run(['cc','-std=c11','-O2','-Ifirmware','tests/bench_v59_poly.c',src,'-o',exe],cwd=root,check=True)
  exes[name]=exe
 samples={}
 for trial in range(8):
  for name in (list(exes) if trial%2==0 else list(reversed(exes))):
   for line in subprocess.check_output([exes[name],'320'],text=True).splitlines():
    voices,mode,stage,digest,time=line.split()
    key=int(voices),int(mode),int(stage),digest
    samples.setdefault(key,{}).setdefault(name,[]).append(float(time))
 writer=csv.writer(sys.stdout,lineterminator='\n')
 writer.writerow(['voices','osc','phase','v58_us_per_block','v59_us_per_block','percent_less_time'])
 for (voices,mode,stage,digest),by_version in sorted(samples.items()):
  a,b=[statistics.median(by_version[name])/1000 for name in exes]
  writer.writerow([voices,['ORGAN','SAW'][mode],['attack','sustain','release','changes'][stage],f'{a:.2f}',f'{b:.2f}',f'{100*(1-b/a):.1f}'])
