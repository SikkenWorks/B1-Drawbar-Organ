#!/usr/bin/env python3
"""Build and audit the *experimental* B1 FM-1 OTA candidate from one user-supplied BEETLE RC1.01 .fwsc.

This does not flash hardware. Never claim independent recovery or real hardware
boot validation based on this host-side static check.
"""
from pathlib import Path
import argparse,hashlib,json,sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from fm1_fwsc import (verify_fwsc,parse_ufw,extract_stock_assets,build_flash_image_from_assets,
                      build_fwsc,validate_app_binary)
SOURCE_SHA='2b79c6bf29fa65adb6f061aae4dfea23e13b4abf3829c96aacaee1abbc1e55de'
PRODUCT='FM-1_481026'

def sha(data):return hashlib.sha256(data).hexdigest()

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--reference',type=Path,required=True,help='Exact 1.01 RC1.01 BEETLE .fwsc (SHA-pinned)')
 p.add_argument('--app',type=Path,required=True)
 p.add_argument('--out',type=Path,required=True)
 args=p.parse_args()
 if args.out.suffix!='.fwsc':p.error('Output must end in .fwsc')
 if sha(args.reference.read_bytes())!=SOURCE_SHA: p.error('Reference BEETLE package SHA-256 does not match this audited source. No output created.')
 report,errors=verify_fwsc(args.reference)
 if errors or report['product']!='FM-1_2101':p.error('BEETLE reference invalid: '+str(errors))
 app=args.app.read_bytes()
 errors=validate_app_binary(app)
 if errors:p.error('App failed entry audit: '+str(errors))
 assets=extract_stock_assets(args.reference)
 old=next(e['data'] for e in parse_ufw(args.reference.read_bytes())['entries'] if e['name']=='flash.bin')
 flash=bytearray(build_flash_image_from_assets(app,assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload']))
 flash[:0x4000]=old[:0x4000]  # byte-for-byte protected boot region of this reference
 pkg=build_fwsc(bytes(flash),assets['ota'],PRODUCT)
 args.out.parent.mkdir(parents=True,exist_ok=True)
 args.out.write_bytes(pkg)
 candidate,errors=verify_fwsc(args.out)
 if errors or not candidate['flash']['app_crc_ok']:
  args.out.unlink(missing_ok=True)
  p.error('generated package does not pass CRC/app audits: '+str(errors))
 new=next(e['data'] for e in parse_ufw(pkg)['entries'] if e['name']=='flash.bin')
 if new[:0x4000]!=old[:0x4000]:raise AssertionError('critical 16KiB header mismatch')
 print('Candidate:',args.out,'identity',PRODUCT,'bytes',len(pkg),'sha256',sha(pkg))
 print('Boot header SHA-256:',sha(new[:0x4000]),'byte-identical to supplied BEETLE reference')
 print('WARNING: Tested offline only. Hardware boot/OTA/rollback not proven.')
 return 0
if __name__=='__main__':raise SystemExit(main())
