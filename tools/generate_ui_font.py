#!/usr/bin/env python3
"""Offline bitmap compiler for the B1 on-device 2bpp proportional fonts.

Generated glyph pixels are embedded in firmware/b1_font.h. The instrument
never loads a TTF/OTF file and no font files are distributed.
Source: Inter by The Inter Project Authors, under SIL OFL 1.1.
"""
from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
FONT_ROOT=Path('/usr/share/fonts/opentype/inter')
TARGET=Path(__file__).resolve().parent.parent/'firmware'/'b1_font.h'
CHARS=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./:-+%<>!_?()#'"
STYLES=[('small','Inter-Medium.otf',11,15),('headline','Inter-SemiBold.otf',18,24)]
lines=['/* Generated 2bpp proportional font bitmap; SikkenWorks B1 V0.5.',' * Source design: Inter by The Inter Project Authors (2016-2023).', ' * See third_party/INTER-OFL-1.1.txt. No font files distributed.', ' * All characters rasterized at build time and packed for embedded FM1 LCD.',' */','#ifndef B1_FONT_H','#define B1_FONT_H','#include <stdint.h>',f'#define B1_FONT_CHARACTERS "{CHARS}"']
for style,fname,size,line_height in STYLES:
    font=ImageFont.truetype(str(FONT_ROOT/fname),size)
    ascent,descent=font.getmetrics()
    matrix=[]
    char_info=[]
    packed=[]
    for char in CHARS:
        bbox=font.getbbox(char)
        w=int(max(1,bbox[2]-bbox[0]));h=max(0,int(bbox[3]-bbox[1]))
        if char == ' ':w=max(3,round(size*.31));h=0
        im=Image.new('L',(max(1,w),max(1,h)),0)
        if h:
            draw=ImageDraw.Draw(im)
            draw.text((-bbox[0],-bbox[1]),char,font=font,fill=255)
        offset=len(packed)
        gray=list(im.get_flattened_data()) if h else []
        for k in range(0,len(gray),4):
            px=gray[k:k+4]
            vals=[(0 if v<28 else 1 if v<105 else 2 if v<190 else 3) for v in px]
            packed.append(sum(vals[j]<<(6-2*j) for j in range(len(vals))))
        # Y relative to font line top for consistent baselines (including descenders)
        yoff=bbox[1]-(ascent-size) - 1 if h else 0
        advance=round(font.getlength(char)) + (0 if size==11 else 1)
        char_info.append((w,h,yoff,max(advance,w+1),offset))
    lines.append(f'#define B1_FONT_{style.upper()}_HEIGHT {line_height}')
    lines.append(f'typedef struct {{uint8_t w,h,y,adv;uint16_t off;}} b1_font_{style}_glyph_t;')
    lines.append(f'static const b1_font_{style}_glyph_t b1_font_{style}_glyphs[{len(CHARS)}]={{')
    for i,(w,h,y,advance,offset) in enumerate(char_info):
        if not (-1<=y<255):raise ValueError((style,CHARS[i],y))
        lines.append(f' {{{w},{h},{y},{advance},{offset}}}, /* {repr(CHARS[i])} */')
    lines.append('};')
    lines.append(f'static const uint8_t b1_font_{style}_pixels[{len(packed)}]={{')
    for k in range(0,len(packed),22):lines.append(' '+','.join(str(v) for v in packed[k:k+22])+',')
    lines.append('};')
lines.append('#endif')
TARGET.write_text('\n'.join(lines)+'\n')
print('generated font header',TARGET, TARGET.stat().st_size,'bytes')
