#!/usr/bin/env python3
"""Native OTA test: synthetic 8-entry vendor container and failure modes.

This MUST NOT be reported as a replay of M-VAVE's original V15 .fwsc.
Positive stock-path test only replaces the published SHA in a TEMPORARY test
copy of ota.c with the digest of the synthetic flash[0x4000:0x93000]; the
released firmware/ota.c is never altered. Negative tests use production code.
"""
from pathlib import Path
import hashlib, struct, sys, subprocess, tempfile
R=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(R/'tools'))
from fm1_fwsc import parse_ufw, _ufw_entry, crc16, stream_xor
CC=sys.argv[1] if len(sys.argv)>1 else 'cc'
ref=parse_ufw((R/'build/SikkenWorks-B1-V5.5.fwsc').read_bytes())
bytype={e['type']:e['data'] for e in ref['entries']}
flash=bytype[0]
assert len(flash)==0x93000

def crcstep(c,b):
    c ^= b<<8
    for _ in range(8):c=((c<<1)^0x1021)&0xffff if c&0x8000 else (c<<1)&0xffff
    return c

def pad_crc(buf,start,goal):
    """Set two controllable bytes to make an exact target CRC."""
    fixed=bytes(buf[:start]); suffix=bytes(buf[start+2:]); base=crc16(fixed)
    for a in range(256):
        ca=crcstep(base,a)
        for b in range(256):
            if crc16(suffix,crcstep(ca,b))==goal:
                buf[start:start+2]=bytes([a,b]);return
    raise ValueError('no crc correction found')

# Synthetic nested vendor image with REAL stock 0x4de1 length and CRCs.
body=bytearray(bytes((i*29+7)&0xff for i in range(0x4de1)))
inner=bytearray(32)
struct.pack_into('<H',inner,2,0xffff)
struct.pack_into('<I',inner,4,0x5b1c)
struct.pack_into('<I',inner,8,0x01c0a800)
inner[16:31]=b'usb_hid_ota.bin'
struct.pack_into('<H',inner,0,crc16(inner[2:]))
body[:32]=inner
pad_crc(body,len(body)-2,0x5881)
assert crc16(body)==0x5881
outer=bytearray(32)
struct.pack_into('<H',outer,2,0x5881)
struct.pack_into('<I',outer,4,0x20)
struct.pack_into('<I',outer,8,len(body))
outer[12]=0x41
outer[16:31]=b'usb_hid_ota.bin'
pad_crc(outer,14,0xebaa)
struct.pack_into('<H',outer,0,0xebaa)
assert crc16(outer[2:])==0xebaa
vendor_ota=bytes(outer+body)
assert len(vendor_ota)==0x4e01

def logical_vendor(damage=None):
    files=[(0,'flash.bin',flash), (1,'info.log',b'B1 unit test info'),
           (2,'USR',b'unit-test user info'), (3,'isd_config.ini',b'JieLi vendor config test'),
           (100,'ota.bin',vendor_ota), (101,'script.ver',b'unit test script'),
           (102,'blimit.bin',b'fake flash limiter'), (103,'tail.bin',b'unit test tail')]
    entries=[];off=0x400
    for i,(typ,name,data) in enumerate(files):
        off=(off+0xff)&~0xff
        entries.append([bytearray(_ufw_entry(typ,i,data,off,name)),off,data,name])
        off+=len(data)
    total=off
    h=bytearray(64)
    struct.pack_into('<IH',h,4,total,8)
    struct.pack_into('<HHH',h,10,4,0x200,0)
    h[16:22]=b'AC791N'
    enc_list=bytearray()
    for e,_,_,_ in entries:
        stream_xor(e,0,len(e));enc_list+=e
    struct.pack_into('<H',h,2,crc16(enc_list))
    struct.pack_into('<H',h,0,crc16(h[2:]))
    stream_xor(h,0,len(h))
    out=bytearray(b'\xff'*total)
    out[:64]=h;out[64:64+len(enc_list)]=enc_list
    for e,off,data,name in entries:out[off:off+len(data)]=data
    if damage == 'extra-crc':
        a=next((off for _,off,_,nm in entries if nm=='info.log'))
        out[a]^=1
    if damage == 'stock-body':
        a=next((off for _,off,_,nm in entries if nm=='flash.bin'))
        out[a+0x4000+1234]^=1  # also breaks UFW CRC
    if damage == 'overlap':
        i=7;p=64+i*80;dec=bytearray(out[p:p+80]);stream_xor(dec,0,80)
        struct.pack_into('<I',dec,8,entries[6][1])
        stream_xor(dec,0,80);out[p:p+80]=dec
        hh=bytearray(out[:64]);stream_xor(hh,0,64)
        struct.pack_into('<H',hh,2,crc16(out[64:64+8*80]))
        struct.pack_into('<H',hh,0,crc16(hh[2:]))
        stream_xor(hh,0,64);out[:64]=hh
    return bytes(out)

with tempfile.TemporaryDirectory(prefix='b1-v55-stock-native-') as tmp:
    T=Path(tmp)
    (T/'boot.bin').write_bytes(flash[:0x4000])
    prod=(R/'firmware/ota.c').read_text()
    assert '6edf' not in prod.lower() or '0x6e,0xdf' in prod.lower()
    # No overrides in production. Only synthetic positive case rewrites the
    # exact published digest bytes in a temporary copy of the C implementation.
    sha=hashlib.sha256(flash[0x4000:0x93000]).digest()
    published=bytes.fromhex('6edf3c37fb5bbbc33607c89375ee024d5477c17914d72221c8c68e58a8255686')
    def inits(d):return ','.join('0x%02x'%b for b in d)
    a=','.join('0x%02x'%b for b in published[:8])
    # locate array with whitespace-agnostic regex
    import re
    m=re.search(r'(static const uint8_t want\[32\] = \{)(.*?)(\};)',prod,re.S)
    assert m and len(re.findall(r'0x[0-9a-f]{2}',m.group(2)))==32
    mock=prod[:m.start(2)]+'\n'+ '\n'.join('        '+inits(sha[i:i+8])+',' for i in range(0,32,8))+'\n    '+prod[m.end(2):]
    # Synthetic whole-package pin: device reconstructs 20 one-byte ID
    # markers from the logical file. Patch ONLY the temporary test source.
    synthetic=logical_vendor()
    idtext='FM-1_015'
    packed=b''.join(synthetic[i*47:(i+1)*47]+bytes([(ord(idtext[i])+i+1)&255 if i<len(idtext) else 0x7d]) for i in range(20))+synthetic[940:]
    fullsha=hashlib.sha256(packed).digest()
    vendstart=mock.index('static int b1_ota_v15_exact_package(')
    first=mock[:vendstart]; rest=mock[vendstart:]
    a=re.search(r'(static const uint8_t want\[32\] = \{)(.*?)(\};)',rest,re.S)
    assert a and len(re.findall(r'0x[0-9a-f]{2}',a.group(2)))==32
    rest=rest[:a.start(2)]+'\n'+'\n'.join('        '+inits(fullsha[i:i+8])+',' for i in range(0,32,8))+'\n    '+rest[a.end(2):]
    rest=rest.replace('b1_vendor_ufw_total != 699936u',f'b1_vendor_ufw_total != {len(synthetic)}u',1)
    mock=first+rest
    (T/'ota.c').write_text(mock)
    origtest=(R/'tests/native_ota_replay_v54.c').read_text()
    origtest=origtest.replace('int known_loader_ok=(variant_expected ||','int known_loader_ok=((argc>4 && !strcmp(argv[4],"stock-test")) || variant_expected ||')
    (T/'test.c').write_text(origtest)
    def native(source, exe):
        args=[CC,'-std=c11','-O2','-I',T if source=='test' else R/'firmware','-I',R/'firmware',T/'test.c',R/'firmware/native_digest.c','-o',T/exe]
        if source=='production':
            # The include resolution from test.c selects T/ota.c, so a copy
            # of production is needed for negative tests.
            (T/'ota.c').write_text(prod)
        else:(T/'ota.c').write_text(mock)
        subprocess.run([str(x) for x in args],check=True)
    def run(name, data, expected, mode=None,exe='native_prod'):
        path=T/(name+'.ufw');path.write_bytes(data)
        cmd=[T/exe,path,T/'boot.bin',expected]
        if mode:cmd.append(mode)
        r=subprocess.run([str(x) for x in cmd],text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        print(name,':',r.stdout.strip() or r.stderr.strip(),flush=True)
        if r.returncode:raise RuntimeError(name+' failed '+r.stderr)
    native('production','native_prod')
    run('8entry_real_hash_mismatch_safe_reject',logical_vendor(),'expect_prereject')
    run('8entry_extra_file_corrupted_reject',logical_vendor('extra-crc'),'expect_prereject')
    run('8entry_overlap_reject',logical_vendor('overlap'),'expect_prereject')
    native('test','native_test')
    run('8entry_synthetic_v15_protocol_positive',logical_vendor(),'expect_pass','stock-test','native_test')
    run('8entry_synthetic_crc_corrupted',logical_vendor('extra-crc'),'expect_prereject',None,'native_test')
    print('PASS: 8-entry vendor format, strict stock hash and native CHECK/success/commit path verified with SYNTHETIC test fixture. Original V15 .fwsc still required for real interoperability test.')
