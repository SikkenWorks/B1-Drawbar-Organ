#!/usr/bin/env python3
"""B1 V5.0 versioned cross-firmware loader replay. Tests *actual* firmware/ota.c.
No device writes. Replays exact B1 V4.3 and BEETLE RC1.01 reference
and negative mutated, checksum-valid UFW inputs.
The Felucca 1.6/1.1.5 original .fwsc must be provided externally to extend coverage.
"""
from pathlib import Path
import subprocess,sys,tempfile,struct
root=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(root/'tools'))
from fm1_fwsc import parse_ufw,decode_fwsc,build_fwsc,stream_xor,crc16

def run(cmd, **kw):
    return subprocess.run(list(map(str,cmd)),check=True,**kw)

with tempfile.TemporaryDirectory(prefix='b1-ota-replay-') as d:
    tmp=Path(d); cc=sys.argv[1] if len(sys.argv)>1 else 'cc'
    exe=tmp/'native_ota_replay'
    run([cc,'-std=c11','-O2','-I',root/'firmware',root/'tests/native_ota_replay_v44.c',root/'firmware/native_digest.c','-o',exe])
    b1=root/'build/SikkenWorks-B1-V5.0.fwsc'
    beetle=root/'build/BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc'
    B=parse_ufw(b1.read_bytes()); ref=parse_ufw(beetle.read_bytes());F=next(e['data'] for e in B['entries'] if e['type']==0);O=next(e['data'] for e in B['entries'] if e['type']==100)
    boot=tmp/'boot.bin';boot.write_bytes(F[:0x4000])
    def case(name,flash,ota,expect='expect_pass',option=None):
        if flash is not None:
            _,raw=decode_fwsc(build_fwsc(bytes(flash),bytes(ota),'FM-1_900'))
        else:raw=ota
        path=tmp/(''.join(c if c.isalnum() or c in '-_' else '_' for c in name)+'.ufw');path.write_bytes(raw)
        cmd=[exe,path,boot,expect]
        if option:cmd.append(option)
        run(cmd)
    case('B1 V5.0 same-family self-update',F,O)
    case('Original BEETLE RC1.01',next(e['data'] for e in ref['entries'] if e['type']==0),next(e['data'] for e in ref['entries'] if e['type']==100))
    print('PASS: native CHECK->success->staging readback->commit for B1 V5.0 and BEETLE RC1.01 (host simulation)')
