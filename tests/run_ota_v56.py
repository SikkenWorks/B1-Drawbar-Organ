#!/usr/bin/env python3
"""Replay the production B1 OTA core against real packages. No hardware access."""
from pathlib import Path
import sys,subprocess,tempfile
R=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(R/'tools'))
from fm1_fwsc import parse_ufw,decode_fwsc,build_fwsc
with tempfile.TemporaryDirectory() as d:
 t=Path(d);exe=t/'ota'
 subprocess.run(['cc','-std=c11','-O2','-I',str(R/'firmware'),str(R/'tests/native_ota_replay_v56.c'),str(R/'firmware/native_digest.c'),'-o',str(exe)],check=True)
 b1=R/'build/SikkenWorks-B1-V5.6.fwsc'
 flash=next(e['data'] for e in parse_ufw(b1.read_bytes())['entries'] if e['type']==0)
 boot=t/'boot';boot.write_bytes(flash[:0x4000])
 def case(name,path,expect='expect_pass',option=None,raw=None):
  data=decode_fwsc(path.read_bytes())[1] if raw is None else raw
  ufw=t/(name+'.ufw');ufw.write_bytes(data)
  subprocess.run([str(exe),str(ufw),str(boot),expect]+([option] if option else []),check=True)
 for name,path in [('B1-self',b1),('B1-5_5',R/'build/SikkenWorks-B1-V5.5.fwsc'),('BEETLE',R/'build/BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc'),('RUPTURE',R/'tests/fixtures/RUPTURE-V1.52.fwsc'),('V15',R/'tests/fixtures/FM1-V15-ORIGINAL.fwsc')]:
  case(name,path)
  case(name+'-residual-loader',path,option='previous-stage')
 for option in ['no-success','cancel','erase-fail','readback-corrupt']:
  case(option,b1,'expect_reject',option)
 case('unknown-stage',b1,'expect_prereject','unknown-stage')
 entries={e['type']:e['data'] for e in parse_ufw(b1.read_bytes())['entries']}
 damaged=bytearray(entries[0]);damaged[0x5000]^=1
 raw=decode_fwsc(build_fwsc(bytes(damaged),entries[100],'FM-1_481026'))[1]
 case('bad-app-valid-container',b1,'expect_prereject',raw=raw)
 raw=bytearray(decode_fwsc(b1.read_bytes())[1]);raw[0x5000]^=1
 case('bad-payload',b1,'expect_prereject',raw=bytes(raw))
 print('PASS: 17 native OTA cases; host simulation only.')
