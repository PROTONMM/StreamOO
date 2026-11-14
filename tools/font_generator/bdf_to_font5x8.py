#!/usr/bin/env python3
"""Convert the pinned public-domain Misc Fixed 5x8 BDF to StreamOO columns."""
import argparse
import hashlib
from pathlib import Path

SOURCE_SHA256 = 'a126e263c6ee843e8725fa3ad630879e74d3294493ce7a00f9a9a21ae10fc2f3'
POLISH = 'ąćęłńóśźżĄĆĘŁŃÓŚŹŻ'


def generate(source):
    raw = Path(source).read_bytes()
    if hashlib.sha256(raw).hexdigest() != SOURCE_SHA256:
        raise ValueError('Source hash differs from the reviewed Misc Fixed 5x8 BDF')
    wanted = set(range(32, 127)) | set(map(ord, POLISH))
    entries = {}
    for block in raw.decode('ascii').split('STARTCHAR ')[1:]:
        lines = block.splitlines()
        code = int(next(line[9:] for line in lines if line.startswith('ENCODING ')))
        if code not in wanted:
            continue
        if code in entries:
            raise ValueError(f'Duplicate glyph U+{code:04X}')
        if 'BBX 5 8 0 -1' not in lines or 'DWIDTH 5 0' not in lines:
            raise ValueError(f'Unexpected geometry for U+{code:04X}')
        bitmap = lines[lines.index('BITMAP') + 1:lines.index('ENDCHAR')]
        if len(bitmap) != 8:
            raise ValueError(f'Unexpected row count for U+{code:04X}')
        pixels = [int(row, 16) for row in bitmap]
        if any(row & 7 or row > 255 for row in pixels):
            raise ValueError(f'Pixels outside five columns for U+{code:04X}')
        # BDF rows: MSB is left. Firmware columns: bit0 is the top row.
        entries[code] = [sum(((row >> (7 - x)) & 1) << y
                             for y, row in enumerate(pixels)) for x in range(5)]
    if entries.keys() != wanted:
        raise ValueError('Missing required ASCII/Polish glyphs')
    rows = ['/* Generated from X11 Misc Fixed Unicode 5x8.bdf (public domain).',
            ' * Source SHA-256: ' + SOURCE_SHA256,
            ' * Source and declaration: assets/fonts/misc-fixed/5x8.bdf',
            ' * Provenance: licenses/Misc-Fixed-NOTICE.md.',
            ' * Regenerate with tools/font_generator/bdf_to_font5x8.py. */',
            '#include "streamoo_font5x8.h"', '#include <string.h>', '',
            'static const struct { uint32_t unicode; uint8_t columns[5]; } glyphs[] = {']
    for code, columns in sorted(entries.items()):
        rows.append('    {0x%04x, {%s}},' % (code, ', '.join(f'0x{byte:02x}' for byte in columns)))
    rows += ['};', '',
             'bool streamoo_font5x8(void *context, uint32_t codepoint, uint8_t columns[5])',
             '{', '    (void)context;', '    if (!columns) return false;',
             '    size_t left = 0, right = sizeof(glyphs) / sizeof(glyphs[0]);',
             '    while (left < right) {', '        const size_t mid = left + (right - left) / 2;',
             '        if (glyphs[mid].unicode < codepoint) left = mid + 1;',
             '        else right = mid;', '    }',
             '    if (left == sizeof(glyphs) / sizeof(glyphs[0]) || glyphs[left].unicode != codepoint)',
             '        return false;', '    memcpy(columns, glyphs[left].columns, 5);',
             '    return true;', '}', '']
    return '\n'.join(rows)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.write_text(generate(args.source))
