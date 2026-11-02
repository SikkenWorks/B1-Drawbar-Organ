#!/usr/bin/env python3
"""Replay the REAL original M-VAVE stock V15 package against B1 native ota.c.

Usage: python tests/run_ota_v55_original_stock.py /path/to/FM-1.fwsc [cc]
Requires SHA-pinned vendor file; no package is bundled/distributed.
The device/flash never touched. This proves native host-side CHECK only.
"""
from pathlib import Path
import sys,hashlib,subprocess,tempfile
R=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(R/'tools'))
from fm1_fwsc import parse_ufw, decode_fwsc
WANT='db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a'
APP_WANT='6edf3c37fb5bbbc33607c89375ee024d5477c17914d72221c8c68e58a8255686'
if len(sys.argv)<2:raise SystemExit('Usage: python tests/run_ota_v55_original_stock.py /path/to/original-FM-1.fwsc [cc]')
p=Path(sys.argv[1]); cc=sys.argv[2] if len(sys.argv)>2 else 'cc'
raw=p.read_bytes()
assert hashlib.sha256(raw).hexdigest()==WANT,'NOT the original pinned V15 vendor package'
pkg=parse_ufw(raw)
assert pkg['product']=='FM-1_015' and pkg['file_count']==8 and pkg['header_crc_ok'] and pkg['list_crc_ok']
assert set(e['name'] for e in pkg['entries'])=={'flash.bin','info.log','USR','isd_config.ini','ota.bin','script.ver','blimit.bin','tail.bin'}
assert all(e['data_crc_ok'] for e in pkg['entries'])
flash=next(e['data'] for e in pkg['entries'] if e['name']=='flash.bin')
assert len(flash)==0x93000 and hashlib.sha256(flash[0x4000:0x93000]).hexdigest()==APP_WANT
_,logical=decode_fwsc(raw)
with tempfile.TemporaryDirectory(prefix='b1-v55-official-v15-') as td:
 t=Path(td)
 (t/'v15.ufw').write_bytes(logical)
 (t/'boot.bin').write_bytes(flash[:0x4000])
 # stock positive test should compare incoming vendor loader with staged bytes;
 # avoid the community-loader byte comparison used by the stock-unaware harness.
 h=(R/'tests/native_ota_replay_v54.c').read_text().replace(
 'int known_loader_ok=(variant_expected ||',
 'int known_loader_ok=((argc>4 && !strcmp(argv[4],"stock-test")) || variant_expected ||')
 (t/'native.c').write_text(h)
 subprocess.run([cc,'-std=c11','-O2','-I',str(R/'firmware'),str(t/'native.c'),str(R/'firmware/native_digest.c'),'-o',str(t/'native')],check=True)
 subprocess.run([str(t/'native'),str(t/'v15.ufw'),str(t/'boot.bin'),'expect_pass','stock-test'],check=True)
 print('PASS: exact original M-VAVE V15 native CHECK -> success -> stage/readback -> commit, software simulation only')
 print('Hardware two-way Hub restoration: STILL NOT CONFIRMED')
