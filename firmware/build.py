#!/usr/bin/env python3
"""Build SikkenWorks B1 native AC791N application. This never flashes hardware."""
import os,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parent.parent
src=root/'firmware'
out=root/'build';out.mkdir(parents=True,exist_ok=True)
tc=Path(os.environ.get('JIELI_TOOLCHAIN','/mnt/data/B1-V1.7-work/jieli'))
cc=tc/'pi32v2/bin/clang'
if not cc.exists():cc=tc/'common/bin/clang'
ld=tc/'pi32v2/bin/ld'
if not cc.exists() or not ld.exists():sys.exit('Missing pi32v2 toolchain. Set JIELI_TOOLCHAIN.')
objects=[]
for name in ['b1_main.c','b1_startup.c','b1_engine.c','b1_performance.c','b1_storage.c','b1_ui.c','native_digest.c','crt0.S','hal/fm1_vec.S','hal/fm1_isr.S']:
 obj=out/(Path(name).stem+'.o')
 cmd=[str(cc),'-target','pi32v2']
 if name.endswith('.c'):cmd+=['-Os','-DB1_STORAGE_DYNAMIC=1','-fno-builtin','-ffunction-sections','-Wall','-I'+str(src/'hal')]
 cmd+=['-c',str(src/name),'-o',str(obj)]
 print('CC',name,flush=True)
 subprocess.run(cmd,check=True);objects.append(str(obj))
 elf=out/'B1.elf'
subprocess.run([str(ld),'--gc-sections','-e','_start','-T',str(src/'app.ld'),*objects,'-o',str(out/'B1.elf')],check=True)
# The linked ELF is a genuine AC791N target. Package generation remains
# gated on proven BEETLE boot-header compatibility and hardware recovery.
print('Built',out/'B1.elf')
