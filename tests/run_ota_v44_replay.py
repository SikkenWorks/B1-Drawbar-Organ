#!/usr/bin/env python3
"""End-to-end native OTA CHECK/commit replay. Tests *actual* firmware/ota.c.
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
    b1=root/'build/SikkenWorks-B1-V4.3-BEETLE-OTA.fwsc'
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
    case('B1 same-family V4.3',F,O)
    case('Original BEETLE RC1.01',next(e['data'] for e in ref['entries'] if e['type']==0),next(e['data'] for e in ref['entries'] if e['type']==100))
    altered=bytearray(F);top=bytearray(altered[:32]);stream_xor(top,0,32);top[4]^=4;struct.pack_into('<H',top,0,crc16(top[2:]));stream_xor(top,0,32);altered[:32]=top
    case('Foreign protected flash-header APP_ONLY',altered,O)
    foreign=bytearray(O);foreign[100]^=1;struct.pack_into('<H',foreign,2,crc16(foreign[32:]));struct.pack_into('<H',foreign,0,crc16(foreign[2:32]));case('Unknown incoming loader never executed',F,foreign)
    bad=bytearray(F);bad[0x4000+0x120+1024]^=7;case('Corrupt XIP app rejected',bad,O,'expect_reject')
    bad=bytearray(F);top=bytearray(bad[:160]);
    for i in range(0,160,32):stream_xor(top,i,32)
    isdoff=struct.unpack_from('<I',top,0x44)[0];isdsz=struct.unpack_from('<I',top,0x48)[0]
    bad[isdoff+16]=bad[isdoff+15]^0xff
    struct.pack_into('<H',top,0x42,crc16(bad[isdoff:isdoff+isdsz]));struct.pack_into('<H',top,0x40,crc16(top[0x42:0x60]));
    for i in range(0,160,32):stream_xor(top,i,32)
    bad[:160]=top;case('Wrong chip key rejected',bad,O,'expect_reject')
    # same intact package but interrupted at multiple states
    case('Missing success ACK / cleanup',F,O,'expect_reject','no-success')
    case('User abort / no commit',F,O,'expect_reject','cancel')
    case('Injected erase failure / no commit',F,O,'expect_reject','erase-fail')
    _,logical=decode_fwsc(build_fwsc(F,O,'FM-1_900'))
    hdr=bytearray(logical[:0x400]);e=bytearray(hdr[0x90:0xe0]);stream_xor(e,0,80);struct.pack_into('<I',e,8,0x400);stream_xor(e,0,80);hdr[0x90:0xe0]=e
    h=bytearray(hdr[:64]);stream_xor(h,0,64);struct.pack_into('<H',h,2,crc16(hdr[0x40:0xe0]));struct.pack_into('<H',h,0,crc16(h[2:]));stream_xor(h,0,64);hdr[:64]=h
    case('Overlapping UFW entries rejected',None,bytes(hdr)+logical[0x400:],'expect_reject')
    for f in sys.argv[2:]:
        source=Path(f);pkg=parse_ufw(source.read_bytes());
        ent={e['type']:e['data'] for e in pkg['entries']}
        case('Original external '+source.stem,ent[0],ent[100])
    print('PASS: B1 V4.4 native OTA packet-level replay. NOT a physical flash test.')
