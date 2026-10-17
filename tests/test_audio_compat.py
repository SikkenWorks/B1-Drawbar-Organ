#!/usr/bin/env python3
"""V1.1 native PCM golden test versus V1.0 recorded output, six real DSP fixtures."""
from pathlib import Path
from array import array
import math,sys,subprocess
root=Path(__file__).resolve().parent.parent
exe=root/'build'/'b1-native-pcm-fixture'
subprocess.run(['cc','-O2','-std=c11','-Wall','-Wextra','-Werror','-I'+str(root/'firmware'),str(root/'firmware/b1_engine.c'),str(root/'tests/emit_reference_pcm.c'),'-o',str(exe)],check=True)
for mode in (0,1):
 for voices in (0,4,12):
  name=f'{mode}-{voices}'
  dest=root/'build'/f'v11-{name}.pcm'
  subprocess.run([str(exe),str(mode),str(voices),str(dest)],check=True)
  baseline=array('h');current=array('h')
  baseline.frombytes((root/'tests/golden-v10'/f'v10-{name}.pcm').read_bytes())
  current.frombytes(dest.read_bytes())
  if sys.byteorder!='little': baseline.byteswap();current.byteswap()
  assert len(baseline)==len(current)==8192
  error=sum((int(a)-int(b))**2 for a,b in zip(baseline,current))
  sig=sum(int(a)**2 for a in baseline)
  peak=max(abs(int(a)-int(b)) for a,b in zip(baseline,current))
  snr=10*math.log10(sig/error) if error else float('inf')
  assert peak<=8,(name,peak)
  assert (not sig and not error) or snr>60,(name,snr)
  print(f'{name}: peak PCM delta {peak}/32767; SNR {snr:.2f} dB: PASS')
