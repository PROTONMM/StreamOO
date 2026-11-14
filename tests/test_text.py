import ctypes as C
from pathlib import Path
import random
import subprocess
import tempfile
import unittest
from test_canvas import Canvas, Rect

ROOT = Path(__file__).resolve().parents[1]
GLYPH = C.CFUNCTYPE(C.c_bool, C.c_void_p, C.c_uint32, C.POINTER(C.c_uint8))


class TextTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        library = Path(cls.temp.name) / 'text.so'
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC',
                        '-Icomponents/graphics/include', '-Icomponents/text/include',
                        'components/graphics/streamoo_canvas.c', 'components/text/streamoo_text.c',
                        '-o', str(library)], cwd=ROOT, check=True)
        cls.lib = C.CDLL(str(library))
        cls.lib.streamoo_utf8_next.argtypes = [C.c_char_p, C.c_size_t, C.POINTER(C.c_size_t)]
        cls.lib.streamoo_utf8_next.restype = C.c_uint32
        cls.lib.streamoo_text_width.argtypes = [C.c_char_p, C.c_size_t]
        cls.lib.streamoo_text_width.restype = C.c_size_t
        cls.lib.streamoo_canvas_init.argtypes = [C.POINTER(Canvas), C.POINTER(C.c_uint8),
                                                C.c_size_t, C.c_int16, C.c_int16, C.c_uint16]
        cls.lib.streamoo_canvas_clip.argtypes = [C.POINTER(Canvas), Rect]
        cls.lib.streamoo_text_draw.argtypes = [C.POINTER(Canvas), C.c_int16, C.c_int16,
                                              C.c_char_p, C.c_size_t, C.c_uint8, GLYPH, C.c_void_p]
        cls.lib.streamoo_text_draw.restype = C.c_bool

    def decode(self, data):
        consumed = C.c_size_t(99)
        code = self.lib.streamoo_utf8_next(data, len(data), C.byref(consumed))
        return code, consumed.value

    def test_polish_and_random_unicode_scalars(self):
        scalars = list(map(ord, 'ąćęłńóśźżĄĆĘŁŃÓŚŹŻ')) + [0, 0x7f, 0x80, 0x7ff, 0x800, 0xffff, 0x10000, 0x10ffff]
        rng = random.Random(104)
        scalars += [rng.randrange(0x110000) for _ in range(1000)]
        for scalar in scalars:
            if 0xd800 <= scalar <= 0xdfff:
                continue
            data = chr(scalar).encode('utf-8')
            self.assertEqual(self.decode(data), (scalar, len(data)))

    def test_invalid_utf8_consumes_one_byte(self):
        for data in (b'\x80', b'\xc0\x80', b'\xc1\xbf', b'\xc2', b'\xe2\x82',
                     b'\xe0\x80\x80', b'\xed\xa0\x80', b'\xf0\x80\x80\x80',
                     b'\xf4\x90\x80\x80', b'\xf5\x80\x80\x80', b'\xff', b'\xe2(A'):
            self.assertEqual(self.decode(data), (0xfffd, 1))
        self.assertEqual(self.decode(b''), (0xfffd, 0))

    def test_measure_counts_scalars_not_utf8_bytes(self):
        data = 'ąćęłńóśźż ĄĆĘŁŃÓŚŹŻ'.encode()
        self.assertEqual(self.lib.streamoo_text_width(data, len(data)), 19 * 6)
        self.assertEqual(self.lib.streamoo_text_width(b'A\x00B', 3), 18)
        self.assertEqual(self.lib.streamoo_text_width(b'\xe2(\xa1', 3), 18)

    def draw(self, data, x=0, y=0, supported=True):
        self.buffer = (C.c_uint8 * 130)(*([0xa5] * 130))
        pixels = C.cast(C.byref(self.buffer, 1), C.POINTER(C.c_uint8))
        self.canvas = Canvas()
        self.lib.streamoo_canvas_init(C.byref(self.canvas), pixels, 128, 32, 8, 16)
        self.requested = []

        def glyph(context, scalar, columns):
            self.requested.append(scalar)
            # Synthetic test pattern, unrelated to the supplied font bitmap.
            for i, value in enumerate((0x81, 0x42, 0x24, 0x18, 0xff)):
                columns[i] = value
            return supported
        self.glyph = GLYPH(glyph)
        self.assertTrue(self.lib.streamoo_text_draw(C.byref(self.canvas), x, y,
                                                   data, len(data), 15, self.glyph, None))
        self.assertEqual((self.buffer[0], self.buffer[-1]), (0xa5, 0xa5))

    def test_unicode_provider_and_eighth_row(self):
        self.draw('ąŁź'.encode())
        self.assertEqual(self.requested, list(map(ord, 'ąŁź')))
        self.assertEqual(self.buffer[1], 0xf5)  # bit0 top, first column; neighbor preserved
        self.assertEqual(self.buffer[1 + 7 * 16], 0xf5)  # bit7 retained, not clipped to 7 rows
        self.assertEqual(self.buffer[1 + 2], 0xf5)  # x4 painted, gap x5 untouched

    def test_negative_position_and_vertical_clip(self):
        self.draw('ąćęłńóśźż'.encode(), -7, -3)
        self.assertEqual(self.requested[0], ord('ć'))
        self.assertTrue(all(value == 0xa5 for value in self.buffer[1 + 5 * 16:-1]))

    def test_fallback_and_controls(self):
        self.draw('🙂'.encode(), supported=False)
        self.assertEqual(self.requested, [0x1f642])
        self.assertEqual(self.buffer[1 + 4 * 16], 0xf5)  # fallback left border
        self.draw(b' \n\x00')
        self.assertEqual(self.requested, [])
        self.assertTrue(all(value == 0xa5 for value in self.buffer))
