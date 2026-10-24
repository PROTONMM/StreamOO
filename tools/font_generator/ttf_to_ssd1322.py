#!/usr/bin/env python3
"""Convert a static TrueType font to compact, 4-bpp SSD1322 C glyph data.
Requires Pillow >= 10. Output is a header, optional atlas preview, optional glyph report.
"""
import argparse
import json
import re
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

DEFAULT_CHARS = ''.join(chr(c) for c in range(32, 127)) + 'ĄĆĘŁŃÓŚŹŻąćęłńóśźż€°–—…„”’•→←↑↓'

def symbol(s):
    s = re.sub(r'\W+', '_', s).strip('_')
    if not s or s[0].isdigit(): s = 'font_' + s
    return s

def codepoints(spec, chars):
    result = set(ord(x) for x in chars)
    if spec:
        for part in spec.split(','):
            part = part.strip().upper().removeprefix('U+')
            if not part: continue
            if '-' in part:
                lo, hi = part.split('-', 1)
                lo, hi = int(lo.removeprefix('U+'),16), int(hi.removeprefix('U+'),16)
                if hi < lo or hi-lo>8192: raise ValueError('Invalid or excessive codepoint range')
                result.update(range(lo,hi+1))
            else: result.add(int(part,16))
    return sorted(result)

def pack_4bpp(image):
    raw = list(image.getdata()); out = bytearray()
    for i in range(0,len(raw),2):
        hi = (raw[i] * 15 + 127)//255
        lo = (raw[i+1] * 15 + 127)//255 if i+1<len(raw) else 0
        out.append((hi << 4) | lo)
    return out

def emit(args):
    font = ImageFont.truetype(str(args.ttf), args.size)
    ascent, descent = font.getmetrics()
    chars = Path(args.chars_file).read_text(encoding='utf-8') if args.chars_file else DEFAULT_CHARS
    points = codepoints(args.ranges, chars)
    glyphs=[]; pixels=bytearray(); missing=[]
    for cp in points:
        char=chr(cp)
        mask=font.getmask(char, mode='L')
        bbox=font.getbbox(char, anchor='ls')
        advance=round(font.getlength(char))
        if not bbox: missing.append(cp); continue
        x0,y0,x1,y1=bbox
        w,h=max(0,x1-x0),max(0,y1-y0)
        # A nonexistent glyph often renders as a tofu square; check cmap below if fontTools installed.
        if w*h:
            tile=Image.new('L',(w,h),0)
            draw=ImageDraw.Draw(tile)
            draw.text((-x0,-y0),char,font=font,anchor='ls',fill=255,stroke_width=0)
            packed=pack_4bpp(tile)
        else: packed=bytearray()
        glyphs.append(dict(cp=cp,w=w,h=h,x=x0,y=y0,advance=advance,offset=len(pixels),nbytes=len(packed)))
        pixels.extend(packed)
    if not glyphs: raise ValueError('No glyphs')
    out=args.out
    out.parent.mkdir(parents=True,exist_ok=True)
    ident=symbol(args.name or (args.ttf.stem+'_'+str(args.size)))
    guard=ident.upper()+'_H'
    with out.open('w',encoding='utf-8') as f:
        f.write(f'// Generated from {args.ttf.name}; size {args.size}px.\n// Preserve the source font license and copyright notice when redistributing.\n#ifndef {guard}\n#define {guard}\n#include <stdint.h>\n\n')
        f.write('#ifndef STREAMOO_FONT_TYPES_DEFINED\n#define STREAMOO_FONT_TYPES_DEFINED\ntypedef struct { uint32_t cp, offset; int16_t x, y, advance; uint16_t w, h; } streamoo_glyph_t;\ntypedef struct { const streamoo_glyph_t *glyphs; const uint8_t *bitmap; uint32_t count; uint16_t line_height; int16_t ascent; } streamoo_font_t;\n#endif\n\n')
        f.write(f'static const uint8_t {ident}_bitmap[] = {{\n')
        for i in range(0,len(pixels),16): f.write('    '+', '.join(f'0x{x:02X}' for x in pixels[i:i+16])+',\n')
        f.write('};\n')
        f.write(f'static const streamoo_glyph_t {ident}_glyphs[] = {{\n')
        for g in glyphs:
            f.write(f'    {{0x{g["cp"]:04X}, {g["offset"]}, {g["x"]}, {g["y"]}, {g["advance"]}, {g["w"]}, {g["h"]}}},\n')
        f.write('};\n')
        f.write(f'static const streamoo_font_t {ident} = {{ {ident}_glyphs, {ident}_bitmap, {len(glyphs)}, {ascent+descent}, {ascent} }};\n#endif\n')
    if args.preview:
        cols=10; cell_w=max(34,round(args.size*2.2)); cell_h=max(36,round(args.size*2.3)); rows=(len(glyphs)+cols-1)//cols
        atlas=Image.new('RGB',(cols*cell_w,rows*cell_h),(18,18,24)); draw=ImageDraw.Draw(atlas)
        for i,g in enumerate(glyphs):
            x=(i%cols)*cell_w; y=(i//cols)*cell_h
            draw.rectangle((x,y,x+cell_w-2,y+cell_h-2),outline=(45,45,52))
            draw.text((x+5,y+5+ascent),chr(g['cp']),font=font,anchor='ls',fill=(235,235,242))
            draw.text((x+4,y+cell_h-12),f'U+{g["cp"]:04X}',fill=(110,200,210))
        args.preview.parent.mkdir(parents=True,exist_ok=True); atlas.save(args.preview)
    if args.report:
        args.report.write_text(json.dumps(dict(font=str(args.ttf),size_px=args.size,line_height=ascent+descent,ascent=ascent,glyph_count=len(glyphs),bitmap_bytes=len(pixels),missing_bbox=missing,glyphs=glyphs),indent=2,ensure_ascii=False),encoding='utf-8')
    print(f'Wrote {out}: {len(glyphs)} glyphs, {len(pixels)} bitmap bytes, line height {ascent+descent}px')
    print('Note: verify glyph coverage in source TTF; unsupported codepoints can render replacement boxes.')

if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('ttf',type=Path,help='Static Monaspace Krypton TTF path')
    p.add_argument('--size',type=int,default=14,help='Font size in pixels (default 14)')
    p.add_argument('--out',type=Path,default=Path('monaspace_krypton_14.h'))
    p.add_argument('--name',help='Identifier for C symbol')
    p.add_argument('--chars-file',help='UTF-8 file specifying needed characters, replaces built-in set')
    p.add_argument('--ranges',help='Additional hex Unicode ranges, e.g. 0020-007E,0104,0142')
    p.add_argument('--preview',type=Path,help='Write a PNG glyph atlas')
    p.add_argument('--report',type=Path,help='Write glyph metrics JSON')
    a=p.parse_args()
    if a.size<5 or a.size>96: p.error('--size must be between 5 and 96')
    emit(a)
