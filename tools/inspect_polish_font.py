#!/usr/bin/env python3
"""Inspect a user-provided font ZIP without importing font bytes into firmware.

Only the glcdfont.c data initializer is parsed; archive instructions and C code
are never executed. Optional preview is a private build artifact.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
from zipfile import ZipFile
import zlib

SLOTS = {'ą': 0xb8, 'ć': 0xbd, 'ę': 0xd6, 'ł': 0xcf, 'ń': 0xc0,
         'ó': 0xbe, 'ś': 0xcb, 'ź': 0xbb, 'ż': 0xb9,
         'Ą': 0xb7, 'Ć': 0xc4, 'Ę': 0xd7, 'Ł': 0xd0, 'Ń': 0xc1,
         'Ó': 0xbf, 'Ś': 0xcc, 'Ź': 0xbc, 'Ż': 0xba}


def inspect(archive, preview=None):
    with ZipFile(archive) as zipped:
        names = [item for item in zipped.infolist() if item.filename.endswith('/glcdfont.c')]
        if len(names) != 1 or names[0].file_size > 100000:
            raise ValueError('Expected one bounded glcdfont.c')
        license_files = [item.filename for item in zipped.infolist()
                         if any(word in Path(item.filename).name.lower()
                                for word in ('license', 'copying', 'copyright'))]
        raw = zipped.read(names[0])
    source = raw.decode('utf-8-sig')
    source = re.sub(r'/\*.*?\*/|//[^\n]*', '', source, flags=re.S)
    body = re.search(r'font\[\]\s*(?:PROGMEM)?\s*=\s*\{([^}]+)\}', source)
    if not body:
        raise ValueError('Font initializer not found')
    values = bytes(int(token, 0) for token in re.findall(r'0[xX][0-9a-fA-F]+|0[bB][01]+', body[1]))
    if len(values) != 256 * 5:
        raise ValueError(f'Expected 256 five-column glyphs, found {len(values)} bytes')
    mapping = [{'char': char, 'unicode': f'U+{ord(char):04X}', 'slot': f'0x{slot:02X}',
                'uses_row_8': any(value & 128 for value in values[slot * 5:slot * 5 + 5])}
               for char, slot in SLOTS.items()]
    if preview:
        width, height = 216, 56
        pixels = bytearray(width * height)

        def draw(slot, x, y, scale):
            for col, value in enumerate(values[slot * 5:slot * 5 + 5]):
                for row in range(8):
                    if value & (1 << row):
                        for dy in range(scale):
                            for dx in range(scale):
                                pixels[(y + row * scale + dy) * width + x + col * scale + dx] = 255
        for index, slot in enumerate(SLOTS.values()):
            x, y = (index % 9) * 24 + 4, (index // 9) * 28
            draw(slot, x, y, 2)
            for column, char in enumerate(f'{slot:02X}'):
                draw(ord(char), x + column * 6, y + 18, 1)

        def chunk(kind, data):
            return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
        rows = b''.join(b'\0' + pixels[y * width:(y + 1) * width] for y in range(height))
        png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 0, 0, 0, 0))
        Path(preview).write_bytes(png + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b''))
    return {'file': names[0].filename, 'sha256': hashlib.sha256(raw).hexdigest(),
            'glyphs': 256, 'columns': 5, 'stored_rows': 8,
            'license_files': license_files, 'mapping': mapping}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive')
    parser.add_argument('--preview')
    args = parser.parse_args()
    print(json.dumps(inspect(args.archive, args.preview), ensure_ascii=False, indent=2))
