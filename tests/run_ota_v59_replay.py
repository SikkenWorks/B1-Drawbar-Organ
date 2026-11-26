#!/usr/bin/env python3
"""V5.9 actual native ota.c host transport replay. No real hardware writes."""
from pathlib import Path
import subprocess, sys, tempfile, struct
root=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(root/'tools'))
from fm1_fwsc import parse_ufw,decode_fwsc,build_fwsc,stream_xor,crc16
with tempfile.TemporaryDirectory(prefix='b1-v59-ota-') as dirname:
    tmp=Path(dirname); exe=tmp/'native_ota_replay'
    subprocess.run(['cc','-O2','-std=c11','-I',str(root/'firmware'),str(root/'tests/native_ota_replay_v44.c'),str(root/'firmware/native_digest.c'),'-o',str(exe)],check=True)
    primary=parse_ufw((root/'build/SikkenWorks-B1-V5.9.fwsc').read_bytes())
    beetle=parse_ufw((root/'build/BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc').read_bytes())
    boot=tmp/'boot.bin';boot.write_bytes(next(e['data'] for e in primary['entries'] if e['type']==0)[:0x4000])
    def case(label,src,expected='expect_pass',fault=None):
        path=tmp/(label.replace(' ','_')+'.ufw')
        if isinstance(src, bytes):
            _,logical=decode_fwsc(src)
        else:
            f,o=src
            _,logical=decode_fwsc(build_fwsc(bytes(f),bytes(o),'FM-1_481026'))
        path.write_bytes(logical)
        args=[str(exe),str(path),str(boot),expected]
        if fault:args.append(fault)
        proc=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        print(proc.stdout.strip(),flush=True)
        if proc.returncode:raise RuntimeError(label+' native replay failed: '+str(proc.returncode))
    f=next(e['data'] for e in primary['entries'] if e['type']==0)
    o=next(e['data'] for e in primary['entries'] if e['type']==100)
    bf=next(e['data'] for e in beetle['entries'] if e['type']==0)
    bo=next(e['data'] for e in beetle['entries'] if e['type']==100)
    case('B1_V59_self', (f,o))
    case('BEETLE_RC101', (bf,bo))
    case('B1_V52_previous', (next(e['data'] for e in parse_ufw((root/'build/SikkenWorks-B1-V5.2.fwsc').read_bytes())['entries'] if e['type']==0),o))
    # Original unmodified vendor 8-entry archive is required for stock V15.
    official=root/'fixtures/original_MVAVE_Open_V15.fm1.fwsc'
    if official.is_file():
        vendor=official.read_bytes()
        case('Official_Open_V15_8entry',vendor,'expect_pass','stock')
        case('Official_V15_UCUN_FULLFLASH_BLOCKED',vendor,'expect_prereject','ucun')
    else:print('NOT TESTED: original vendor V15 external fixture missing')
    variant=bytearray(o);variant[100]^=1
    struct.pack_into('<H',variant,2,crc16(variant[32:]))
    struct.pack_into('<H',variant,0,crc16(variant[2:32]))
    case('Foreign_recompiled_loader_REJECT',(f,variant),'expect_prereject')
    variant2=bytearray(o)+bytes([0xff])*64
    struct.pack_into('<I',variant2,8,len(variant2)-32)
    struct.pack_into('<H',variant2,2,crc16(variant2[32:]))
    struct.pack_into('<H',variant2,0,crc16(variant2[2:32]))
    case('Foreign_longer_loader_REJECT',(f,variant2),'expect_prereject')
    for label,fault in [('No_success','no-success'),('Abort','cancel'),('Erase_failure','erase-fail'),('Readback_corrupt','readback-corrupt')]:
        case(label,(f,o),'expect_reject',fault)
    case('Unknown_V16_boot_REJECT',(f,o),'expect_prereject','boot-v16')
    case('Bad_XIP',(bytearray(f[:0x5000])+bytearray([f[0x5000]^0x10])+f[0x5001:],o),'expect_prereject')
print('V5.9 OTA replay completed: successful transfers are HOST simulations, not FM1 hardware proof.')
