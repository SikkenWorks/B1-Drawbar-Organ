#!/usr/bin/env python3
"""Validate B1 UI artifacts directly from raw RGB565 and the declared pixel-perfect PNGs."""
import json,hashlib
from pathlib import Path
from PIL import Image
root=Path(__file__).resolve().parent.parent
cap=root/'ui-captures'
records=json.loads((cap/'CAPTURE-MANIFEST.json').read_text())
assert len(records)==64,(len(records),'screens')
for r in records:
    raw=(cap/(r['name']+'.rgb565')).read_bytes()
    assert len(raw)==240*240*2, (r['name'],'wrong raw size')
    assert hashlib.sha256(raw).hexdigest()==r['raw_sha256']
    with Image.open(cap/r['png']) as im:
        assert im.size==(240,240) and im.mode=='RGB'
        img=im.tobytes()
        for i in range(240*240):
            v=raw[i*2]<<8|raw[i*2+1]
            rgb=bytes((((v>>11)&31)*255//31,((v>>5)&63)*255//63,(v&31)*255//31))
            if img[3*i:3*i+3]!=rgb:raise AssertionError((r['name'],i,'PNG pixel differs from actual native framebuffer'))
    assert len(set(raw))>15 or r['name'].startswith('boot-'),(r['name'],'suspicious blank image')
print('Exact 240x240 framebuffer -> PNG comparisons: 64/64 PASS')
