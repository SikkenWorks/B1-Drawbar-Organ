#!/usr/bin/env python3
"""Recreate V3.8 reference PCM with current engine and compare all 16 scenarios.

This tests deterministic host PCM identity, NOT processor deadlines or hardware boot.
"""
from pathlib import Path
import hashlib,os,subprocess,tempfile
root=Path(__file__).resolve().parent.parent
expected='bdb7952c9ab6672f93c0aeba74d90d8db444af8ee8d797c32a91bf356b621761'
cc=os.environ.get('HOST_CC','cc')
with tempfile.TemporaryDirectory(prefix='b1-v39-pcm-') as td:
 exe=Path(td)/'emit_pcm'
 subprocess.run([cc,'-std=c11','-O3','-I',str(root/'firmware'),
                 str(root/'tests/emit_v39_pcm_compat.c'),str(root/'firmware/b1_engine.c'),
                 '-o',str(exe)],check=True)
 pcm=subprocess.check_output([str(exe)])
 actual=hashlib.sha256(pcm).hexdigest()
 assert len(pcm)==1048576,(len(pcm),'unexpected reference size')
 assert actual==expected,(actual,expected)
print('PASS V3.8 -> V3.9: 1,048,576 PCM bytes match 16-mode exact reference SHA-256')
