#!/usr/bin/env python3
"""V1.2 output regression: deterministic code, no clipping, intentional tonal change.
The tests DO NOT claim similarity to an actual Hammond or guitar amplifier.
"""
from pathlib import Path
from array import array
import sys, subprocess, hashlib, math
ROOT=Path(__file__).resolve().parent.parent
EXE=ROOT/'build'/'b1-native-pcm-fixture'
subprocess.run(['cc','-O2','-std=c11','-Wall','-Wextra','-Werror','-I'+str(ROOT/'firmware'),str(ROOT/'firmware/b1_engine.c'),str(ROOT/'tests/emit_reference_pcm.c'),'-o',str(EXE)],check=True)
base=Path('/mnt/data/SikkenWorks-B1-V1.1/build')
for mode in (0,1):
 for voices in (0,4,12):
  name=f'{mode}-{voices}'
  output=ROOT/'build'/f'v12-{name}.pcm'
  subprocess.run([str(EXE),str(mode),str(voices),str(output)],check=True)
  arr=array('h');arr.frombytes(output.read_bytes())
  if sys.byteorder!='little':arr.byteswap()
  assert len(arr)==8192
  if voices==0:assert max(map(abs,arr))==0
  else:
   assert max(map(abs,arr))<32500,(name,'headroom')
   assert sum(int(v)*int(v) for v in arr)>100000,(name,'silence')
   assert max(abs(int(arr[i])-int(arr[i-1])) for i in range(1,len(arr)))<55000
  previous=base/f'v11-{name}.pcm'
  if previous.exists():
   prior=array('h');prior.frombytes(previous.read_bytes())
   if sys.byteorder!='little':prior.byteswap()
   delta=sum(abs(int(x)-int(y)) for x,y in zip(prior,arr))
   if voices:assert delta>0,(name,'intentional sound revisions not audible in PCM')
  sha=hashlib.sha256(output.read_bytes()).hexdigest()
  subprocess.run([str(EXE),str(mode),str(voices),str(output)],check=True)
  assert sha==hashlib.sha256(output.read_bytes()).hexdigest(),'non-repeatable PCM'
  print('PCM',name,'PASS','peak',max(map(abs,arr)),'sha',sha[:12])
print('B1 V1.2 PCM intent, bounded output, zero-voice silence, deterministic fixtures PASS')
