#!/usr/bin/env python3
"""Rasterize a reviewed TTF/OTF to row-aligned 4bpp font data. Build tool only.

Requires Pillow >=10 and fontTools >=4. Coverage is checked against cmap.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

from PIL import Image, ImageDraw, ImageFont, features
import PIL
from fontTools.ttLib import TTFont
import fontTools

DEFAULT_CHARS = ''.join(chr(c) for c in range(32, 127)) + 'ĄĆĘŁŃÓŚŹŻąćęłńóśźż'


def symbol(value):
    value = re.sub(r'\W+', '_', value, flags=re.ASCII).strip('_')
    return ('font_' + value) if not value or value[0].isdigit() else value


def codepoints(spec, chars):
    result = set(map(ord, chars))
    if spec:
        for item in spec.split(','):
            parts = item.strip().upper().split('-')
            bounds = [int(value.removeprefix('U+'), 16) for value in parts]
            if len(bounds) == 1:
                result.add(bounds[0])
            elif len(bounds) == 2 and 0 <= bounds[1] - bounds[0] <= 8192:
                result.update(range(bounds[0], bounds[1] + 1))
            else:
                raise ValueError('Invalid or excessive codepoint range')
    if any(c < 0 or c > 0x10ffff or 0xd800 <= c <= 0xdfff for c in result):
        raise ValueError('Invalid Unicode scalar')
    return sorted(result)


def pack_4bpp(image):
    """High nibble first; odd-width rows have a zero padding nibble."""
    output = bytearray()
    raw = list(image.getdata())
    for y in range(image.height):
        for x in range(0, image.width, 2):
            hi = (raw[y * image.width + x] * 15 + 127) // 255
            lo = (raw[y * image.width + x + 1] * 15 + 127) // 255 if x + 1 < image.width else 0
            output.append((hi << 4) | lo)
    return output


def emit(args):
    if not 5 <= args.size <= 96:
        raise ValueError('Size must be 5..96')
    chars = args.chars_file.read_text(encoding='utf-8') if args.chars_file else DEFAULT_CHARS
    points = codepoints(args.ranges, chars)
    if not points:
        raise ValueError('No characters selected')
    with TTFont(args.ttf) as source:
        cmap = source.getBestCmap()
        missing = [f'U+{cp:04X}' for cp in points if cp not in cmap]
    if missing:
        raise ValueError('Missing source glyphs: ' + ', '.join(missing))
    font = ImageFont.truetype(str(args.ttf), args.size)
    ascent, descent = font.getmetrics()
    glyphs, pixels = [], bytearray()
    for cp in points:
        x0, y0, x1, y1 = font.getbbox(chr(cp), anchor='ls')
        width, height = x1 - x0, y1 - y0
        advance = round(font.getlength(chr(cp)))
        if not (0 < advance <= 32767 and 0 <= width <= 32767 and 0 <= height <= 32767):
            raise ValueError(f'Invalid metrics for U+{cp:04X}')
        if not (-32768 <= x0 <= 32767 and -32768 <= y0 <= 32767):
            raise ValueError(f'Excessive bearing for U+{cp:04X}')
        if y0 < -ascent or y1 > descent:
            raise ValueError(f'Glyph U+{cp:04X} exceeds declared line metrics')
        data = bytearray()
        if width and height:
            tile = Image.new('L', (width, height), 0)
            ImageDraw.Draw(tile).text((-x0, -y0), chr(cp), font=font, anchor='ls', fill=255)
            data = pack_4bpp(tile)
        glyphs.append(dict(cp=cp, offset=len(pixels), x=x0, y=y0, advance=advance,
                           w=width, h=height, nbytes=len(data)))
        pixels.extend(data)
    ident = symbol(args.name or 'streamoo_bitmap_font')
    digest = hashlib.sha256(args.ttf.read_bytes()).hexdigest()
    lines = [f'/* Generated from {args.ttf.name}, {args.size}px; source SHA256 {digest}.',
             ' * Derived font name: StreamOO UI16. Copyright (c) 2023 GitHub.',
             ' * Font data licensed under SIL OFL-1.1; see licenses/Monaspace-OFL.txt. */',
             '#pragma once', '#include "streamoo_bitmap_font.h"', '',
             f'static const uint8_t {ident}_bitmap[] = {{']
    lines += ['    ' + ', '.join(f'0x{b:02x}' for b in pixels[i:i + 16]) + ','
              for i in range(0, len(pixels), 16)]
    lines += ['};', f'static const streamoo_glyph_t {ident}_glyphs[] = {{']
    lines += [f'    {{0x{g["cp"]:04x}, {g["offset"]}, {g["x"]}, {g["y"]}, {g["advance"]}, {g["w"]}, {g["h"]}}},'
              for g in glyphs]
    lines += ['};', f'static const streamoo_font_t {ident} = {{',
              f'    {ident}_glyphs, {ident}_bitmap, {len(glyphs)}, {len(pixels)}, {ascent + descent}, {ascent}',
              '};', '']
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text('\n'.join(lines), encoding='utf-8')
    report = dict(font=args.ttf.name, source_sha256=digest, size_px=args.size,
                  derived_name='StreamOO UI16', line_height=ascent + descent, ascent=ascent,
                  glyph_count=len(glyphs), bitmap_bytes=len(pixels), glyphs=glyphs,
                  generator=dict(pillow=PIL.__version__, fonttools=fontTools.__version__,
                                 freetype=features.version('freetype2')))
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    if args.preview:
        cell_w, cell_h, cols = 40, 40, 12
        preview = Image.new('L', (cols * cell_w, ((len(glyphs) + cols - 1) // cols) * cell_h), 0)
        draw = ImageDraw.Draw(preview)
        for index, g in enumerate(glyphs):
            x, y = (index % cols) * cell_w, (index // cols) * cell_h
            draw.text((x + 8, y + 3 + ascent), chr(g['cp']), font=font, anchor='ls', fill=255)
            draw.text((x + 2, y + 26), f'{g["cp"]:04X}', fill=150)
        args.preview.parent.mkdir(parents=True, exist_ok=True)
        preview.save(args.preview)
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('ttf', type=Path)
    parser.add_argument('--size', type=int, default=13)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--name', default='streamoo_ui_16')
    parser.add_argument('--chars-file', type=Path)
    parser.add_argument('--ranges')
    parser.add_argument('--preview', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    report = emit(args)
    print(f'{report["glyph_count"]} glyphs, {report["bitmap_bytes"]} bytes, line height {report["line_height"]}')
