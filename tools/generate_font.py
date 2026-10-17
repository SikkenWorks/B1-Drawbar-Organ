from PIL import Image,ImageDraw,ImageFont
from pathlib import Path
font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf',9)
chars='ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ./:-+%<>!_'
lines=['/* Generated 5x7 pixel font for B1; SikkenWorks 2026. */', '#pragma once', '#include <stdint.h>', 'static const char b1_font_chars[]="'+chars+'";', 'static const uint8_t b1_font_glyphs[][7]={']
for c in chars:
 im=Image.new('L',(7,10));d=ImageDraw.Draw(im);d.text((0,-2),c,fill=255,font=font)
 bits=[]
 for y in range(7):
  acc=0
  for x in range(5):
   if im.getpixel((x+1,y+1))>105:acc|=1<<(4-x)
  bits.append(acc)
 lines.append(' {'+','.join(str(x) for x in bits)+'}, /* '+c+' */')
lines.append('};')
Path(__file__).resolve().parents[1].joinpath('firmware/b1_font.h').write_text('\n'.join(lines)+'\n')
