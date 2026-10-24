import ctypes as C
from pathlib import Path
import random
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class Rect(C.Structure):
    _fields_ = [(key, C.c_int16) for key in ('x', 'y', 'width', 'height')]


class Canvas(C.Structure):
    _fields_ = [('pixels', C.POINTER(C.c_uint8)), ('bytes', C.c_size_t),
                ('width', C.c_int16), ('height', C.c_int16), ('stride', C.c_uint16),
                ('clip', Rect), ('bounds', Rect)]


class CanvasTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        output = Path(cls.temp.name) / 'canvas.so'
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC',
                        '-I', str(ROOT / 'components/graphics/include'), '-I', str(ROOT / 'main'),
                        str(ROOT / 'components/graphics/streamoo_canvas.c'),
                        str(ROOT / 'main/canvas_smoke.c'), '-o', str(output)], check=True)
        cls.lib = C.CDLL(str(output))
        signatures = {
            'init': ([C.POINTER(Canvas), C.POINTER(C.c_uint8), C.c_size_t,
                      C.c_int16, C.c_int16, C.c_uint16], C.c_bool),
            'clip': ([C.POINTER(Canvas), Rect], None),
            'child': ([C.POINTER(Canvas), Rect, C.POINTER(Canvas)], C.c_bool),
            'pixel': ([C.POINTER(Canvas), C.c_int16, C.c_int16, C.c_uint8], None),
            'fill': ([C.POINTER(Canvas), Rect, C.c_uint8], None),
            'outline': ([C.POINTER(Canvas), Rect, C.c_uint8], None),
            'line': ([C.POINTER(Canvas)] + [C.c_int16] * 4 + [C.c_uint8], None),
            'mask': ([C.POINTER(Canvas)] + [C.c_int16] * 4 +
                     [C.POINTER(C.c_uint8), C.c_size_t, C.c_uint16, C.c_uint8], C.c_bool),
        }
        for name, (args, result) in signatures.items():
            function = getattr(cls.lib, 'streamoo_canvas_' + name)
            function.argtypes, function.restype = args, result
        cls.lib.streamoo_canvas_smoke.argtypes = [C.POINTER(C.c_uint8), C.c_size_t]
        cls.lib.streamoo_canvas_smoke.restype = C.c_bool

    def surface(self, width=9, height=7, stride=7):
        self.guard = (C.c_uint8 * (stride * height + 2))(*([0xa5] * (stride * height + 2)))
        self.pointer = C.cast(C.byref(self.guard, 1), C.POINTER(C.c_uint8))
        self.canvas = Canvas()
        self.assertTrue(self.lib.streamoo_canvas_init(C.byref(self.canvas), self.pointer,
                                                      stride * height, width, height, stride))
        return C.byref(self.canvas)

    def assert_guards(self):
        self.assertEqual((self.guard[0], self.guard[-1]), (0xa5, 0xa5))

    def test_nibble_isolation_odd_width_and_padding(self):
        canvas = self.surface()
        self.lib.streamoo_canvas_fill(canvas, Rect(0, 0, 9, 7), 3)
        for y in range(7):
            self.assertEqual(bytes(self.guard[1 + y * 7:8 + y * 7]), b'\x33' * 4 + b'\x35\xa5\xa5')
        self.lib.streamoo_canvas_pixel(canvas, 1, 0, 15)
        self.assertEqual(self.guard[1], 0x3f)
        self.lib.streamoo_canvas_pixel(canvas, 0, 0, 7)
        self.assertEqual(self.guard[1], 0x7f)
        self.assert_guards()

    def test_random_rectangles_against_independent_pixel_oracle(self):
        canvas = self.surface()
        reference = bytearray(self.guard[1:-1])
        rng = random.Random(42)
        for _ in range(600):
            x, y, w, h = (rng.randint(-12, 18) for _ in range(4))
            gray = rng.randrange(16)
            self.lib.streamoo_canvas_fill(canvas, Rect(x, y, w, h), gray)
            for py in range(7):
                for px in range(9):
                    if w > 0 and h > 0 and x <= px < x + w and y <= py < y + h:
                        index, shift = py * 7 + px // 2, 0 if px & 1 else 4
                        reference[index] = (reference[index] & ~(15 << shift)) | gray << shift
            self.assertEqual(bytes(self.guard[1:-1]), reference)
        self.assert_guards()

    def test_nested_clip_cannot_draw_into_other_widgets(self):
        canvas = self.surface()
        self.lib.streamoo_canvas_clip(canvas, Rect(1, 1, 6, 4))
        child = Canvas()
        self.assertTrue(self.lib.streamoo_canvas_child(canvas, Rect(3, -5, 20, 8), C.byref(child)))
        # A widget resetting its clip still cannot escape inherited limits.
        self.lib.streamoo_canvas_clip(C.byref(child), Rect(0, 0, 32767, 32767))
        self.lib.streamoo_canvas_fill(C.byref(child), Rect(-30000, -30000, 32767, 32767), 0)
        for y in range(7):
            for x in range(9):
                value = self.guard[1 + y * 7 + x // 2]
                pixel = (value >> (0 if x & 1 else 4)) & 15
                self.assertEqual(pixel, 0 if 3 <= x < 7 and 1 <= y < 3 else (5 if x & 1 else 10))
        self.assertEqual((self.canvas.clip.x, self.canvas.clip.width), (1, 6))
        self.assert_guards()

    def test_clipped_outline_does_not_invent_edges(self):
        canvas = self.surface()
        self.lib.streamoo_canvas_fill(canvas, Rect(0, 0, 9, 7), 0)
        self.lib.streamoo_canvas_outline(canvas, Rect(-2, -2, 6, 6), 15)
        self.assertEqual(self.guard[1], 0)  # original top/left edges are outside
        self.assertEqual(self.guard[2], 0x0f)  # original right edge x=3
        self.assertEqual(self.guard[1 + 3 * 7], 0xff)  # original bottom y=3
        self.assert_guards()

    def test_lines_all_octants_and_extreme_coordinates(self):
        canvas = self.surface(9, 9, 5)
        self.lib.streamoo_canvas_fill(canvas, Rect(0, 0, 9, 9), 0)
        for x, y in ((0, 0), (4, 0), (8, 0), (8, 4), (8, 8), (4, 8), (0, 8), (0, 4)):
            self.lib.streamoo_canvas_line(canvas, 4, 4, x, y, 15)
            value = self.guard[1 + y * 5 + x // 2]
            self.assertEqual((value >> (0 if x & 1 else 4)) & 15, 15)
        self.lib.streamoo_canvas_line(canvas, -32768, -32768, 32767, 32767, 7)
        for i in range(9):
            value = self.guard[1 + i * 5 + i // 2]
            self.assertEqual((value >> (0 if i & 1 else 4)) & 15, 7)
        self.assert_guards()

    def test_mask_clipping_transparency_and_validation(self):
        canvas = self.surface()
        mask = (C.c_uint8 * 2)(0b10101010, 0b01010101)
        self.assertTrue(self.lib.streamoo_canvas_mask(canvas, -2, 1, 8, 2, mask, 2, 1, 15))
        self.assertEqual(self.guard[1 + 7], 0xf5)
        self.assertEqual(self.guard[1 + 14], 0xaf)
        before = bytes(self.guard)
        self.assertFalse(self.lib.streamoo_canvas_mask(canvas, 0, 0, 8, 2, mask, 1, 1, 15))
        self.assertFalse(self.lib.streamoo_canvas_mask(canvas, 0, 0, 9, 2, mask, 2, 1, 15))
        self.assertEqual(bytes(self.guard), before)
        self.assert_guards()

    def test_empty_clip_and_bad_gray_are_noops(self):
        canvas = self.surface()
        before = bytes(self.guard)
        self.lib.streamoo_canvas_fill(canvas, Rect(0, 0, 9, 7), 16)
        self.lib.streamoo_canvas_clip(canvas, Rect(32767, 32767, 32767, 32767))
        self.lib.streamoo_canvas_fill(canvas, Rect(0, 0, 9, 7), 0)
        self.lib.streamoo_canvas_line(canvas, -32768, -32768, 32767, 32767, 15)
        self.assertEqual(bytes(self.guard), before)

    def test_invalid_surface_is_disabled(self):
        canvas = self.surface()
        for width, height, stride, length in ((0, 1, 1, 1), (1, -1, 1, 1),
                                               (9, 7, 4, 49), (9, 7, 7, 48)):
            self.assertFalse(self.lib.streamoo_canvas_init(canvas, self.pointer, length,
                                                           width, height, stride))
            self.assertFalse(self.canvas.pixels)
            self.lib.streamoo_canvas_fill(canvas, Rect(0, 0, 9, 7), 0)
        self.assertTrue(all(value == 0xa5 for value in self.guard))

    def test_smoke_scene_matches_reviewed_golden_frame(self):
        canvas = self.surface(256, 64, 128)
        self.assertTrue(self.lib.streamoo_canvas_smoke(self.pointer, 8192))
        self.assertEqual(bytes(self.guard[1:-1]), (ROOT / 'tests/fixtures/canvas_smoke.4bpp').read_bytes())
        self.assert_guards()
