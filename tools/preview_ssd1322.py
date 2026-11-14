#!/usr/bin/env python3
"""Render the actual firmware diagnostic frame to PNG, without Python packages."""
import ctypes
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]


def preview(output):
    with tempfile.TemporaryDirectory() as directory:
        library = Path(directory) / 'pattern.so'
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC',
                        '-I', str(ROOT / 'components/ssd1322/include'),
                        str(ROOT / 'components/ssd1322/ssd1322_protocol.c'), '-o', str(library)],
                       check=True)
        code = ctypes.CDLL(str(library))
        code.ssd1322_test_pattern.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t]
        frame = (ctypes.c_uint8 * 8192)()
        if code.ssd1322_test_pattern(frame, len(frame)):
            raise RuntimeError('Firmware pattern generation failed')
        rows = bytearray()
        for y in range(64):
            rows.append(0)  # PNG filter: none
            for value in frame[y * 128:(y + 1) * 128]:
                rows.extend(((value >> 4) * 17, (value & 15) * 17))

    def chunk(kind, data):
        return (struct.pack('>I', len(data)) + kind + data +
                struct.pack('>I', zlib.crc32(kind + data)))

    png = b'\x89PNG\r\n\x1a\n'
    png += chunk(b'IHDR', struct.pack('>IIBBBBB', 256, 64, 8, 0, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b'')
    Path(output).write_bytes(png)


if __name__ == '__main__':
    if len(sys.argv) != 2:
        sys.exit('Usage: python3 tools/preview_ssd1322.py OUTPUT.png')
    preview(sys.argv[1])
