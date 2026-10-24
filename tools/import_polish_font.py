#!/usr/bin/env python3
"""Reproduce the explicitly authorized font import from the pinned user archive.

Modification license remains OPEN. See THIRD_PARTY_NOTICES.md.
"""
import argparse
import hashlib
from pathlib import Path
from inspect_polish_font import load_font, SLOTS

SOURCE_SHA256 = 'b837c600bebad57c18de4f5f6b2f1da6874a206a3b225d4b63e0e73e6729c7da'


def generate(archive):
    raw, values, _, _ = load_font(archive)
    if hashlib.sha256(raw).hexdigest() != SOURCE_SHA256:
        raise ValueError('Source hash differs from the reviewed user-provided font')
    entries = {code: values[code * 5:code * 5 + 5] for code in range(32, 127)}
    entries.update({ord(char): values[slot * 5:slot * 5 + 5] for char, slot in SLOTS.items()})
    rows = ['/* Generated bitmap subset from user-provided glcdfont.c.',
            ' * Source SHA-256: ' + SOURCE_SHA256,
            ' * Imported on explicit user instruction, 2026-10-24.',
            ' * Modification license OPEN: see THIRD_PARTY_NOTICES.md.',
            ' * Base attribution: Copyright (c) 2012 Adafruit Industries.',
            ' * Base notice: licenses/Adafruit-GFX-BSD.txt. */',
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
    parser.add_argument('archive')
    parser.add_argument('output')
    args = parser.parse_args()
    Path(args.output).write_text(generate(args.archive))
