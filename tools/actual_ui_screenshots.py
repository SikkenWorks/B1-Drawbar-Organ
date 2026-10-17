#!/usr/bin/env python3
"""Produce image captures of exactly the B1 native 240x240 RGB565 framebuffers.

The C frame generator links firmware/b1_ui.c with the same real font,
engine and performance structs as the AC791N target. Each PNG is a
pixel-for-pixel color expansion of the LCD SPI framebuffer. The sheet is
only a contact grid of those exact PNG images, without invented widgets.
"""
from pathlib import Path
import os,subprocess,sys
from PIL import Image
from hashlib import sha256
import json
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'ui-captures';OUT.mkdir(exist_ok=True)
CC=os.environ.get('CC','cc')
exe=ROOT/'build'/'b1-render-actual-ui'
subprocess.run([CC,'-std=c11','-O2','-Wall','-Wextra','-Werror','-pedantic',
  str(ROOT/'firmware'/'b1_ui.c'),str(ROOT/'firmware'/'b1_engine.c'),
  str(ROOT/'firmware'/'b1_performance.c'),str(ROOT/'tests'/'render_ui_frames.c'),'-o',str(exe)],check=True)
subprocess.run([str(exe),str(OUT)],check=True)
# The startup frames are rendered by the same code linked into the FM1
# firmware, never assembled from invented browser/UI elements.
boot_exe=ROOT/'build'/'b1-render-startup'
subprocess.run([CC,'-std=c11','-O2','-Wall','-Wextra','-Werror','-pedantic',
 str(ROOT/'firmware'/'b1_startup.c'),str(ROOT/'firmware'/'b1_ui.c'),
 str(ROOT/'firmware'/'b1_engine.c'),str(ROOT/'firmware'/'b1_performance.c'),
 str(ROOT/'tests'/'render_startup_frames.c'),'-o',str(boot_exe)],check=True)
keyframes=[0,120,250,450,700,1000,1300,1600,2200,2800,2960,3110,3220]
subprocess.run([str(boot_exe),str(OUT),*map(str,keyframes)],check=True)
previews=[];record=[]
for file in sorted(OUT.glob('*.rgb565')):
 data=file.read_bytes()
 if len(data)!=240*240*2:raise ValueError('incorrect raw framebuffer length')
 rgb=bytearray(240*240*3)
 for i in range(240*240):
  v=(data[i*2]<<8)|data[i*2+1]
  rgb[3*i]=((v>>11)&31)*255//31
  rgb[3*i+1]=((v>>5)&63)*255//63
  rgb[3*i+2]=(v&31)*255//31
 img=Image.frombytes('RGB',(240,240),bytes(rgb))
 png=file.with_suffix('.png');img.save(png,optimize=True)
 previews.append(img);record.append({'name':file.stem,'raw_sha256':sha256(data).hexdigest(),'png':png.name,'pixels':'240x240','origin':'firmware/b1_ui.c'})
# Contact sheet is merely a grid of actual captures, no externally drawn UI.
w=4;rows=(len(previews)+w-1)//w
sheet=Image.new('RGB',(w*240,rows*240),(4,4,4))
for i,img in enumerate(previews):sheet.paste(img,((i%w)*240,(i//w)*240))
sheet.save(OUT/'B1-V2.5-UI-CONTACT-SHEET.png',optimize=True)
(OUT/'CAPTURE-MANIFEST.json').write_text(json.dumps(record,indent=2)+'\n')
# A true animation, every frame individually rendered by the firmware C.
from tempfile import TemporaryDirectory
with TemporaryDirectory() as d:
    timings=list(range(0,3221,55))
    if timings[-1]!=3220:timings.append(3220)
    subprocess.run([str(boot_exe),d,*map(str,timings)],check=True)
    gif_frames=[]
    for ms in timings:
        raw=(Path(d)/f'boot-{ms:04d}.rgb565').read_bytes()
        if len(raw)!=240*240*2:raise ValueError('boot frame wrong size')
        pixels=bytearray(240*240*3)
        for i in range(240*240):
            v=(raw[2*i]<<8)|raw[2*i+1]
            pixels[3*i:3*i+3]=bytes((((v>>11)&31)*255//31,((v>>5)&63)*255//63,(v&31)*255//31))
        gif_frames.append(Image.frombytes('RGB',(240,240),bytes(pixels)))
    gif_frames[0].save(OUT/'B1-V2.5-STARTUP-ANIMATION.gif',save_all=True,
                       append_images=gif_frames[1:],duration=[55]*(len(gif_frames)-1)+[130],loop=0,optimize=False)
print(f'Captured {len(previews)} exact native frames + code-rendered GIF')
