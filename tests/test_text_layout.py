import ctypes as C
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_canvas import Canvas, Rect
from test_text import GLYPH

ROOT = Path(__file__).resolve().parents[1]


class LayoutTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        library = Path(cls.temp.name) / 'layout.so'
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC',
                        '-Icomponents/graphics/include', '-Icomponents/text/include', '-Icomponents/ui_widgets/include', '-Imain',
                        'components/graphics/streamoo_canvas.c', 'components/text/streamoo_text.c',
                        'components/text/streamoo_font5x8.c', 'components/text/streamoo_text_layout.c',
                        'components/text/streamoo_bitmap_font.c', 'components/text/streamoo_ui_font.c',
                        'components/ui_widgets/streamoo_visualization.c', 'main/visualization_probe.c',
                        'main/text_layout_smoke.c', '-o', str(library)], cwd=ROOT, check=True)
        cls.lib = C.CDLL(str(library))
        cls.lib.streamoo_marquee_offset.argtypes = [C.c_uint32, C.c_uint32, C.c_uint64,
                                                   C.c_uint32, C.c_uint32]
        cls.lib.streamoo_marquee_offset.restype = C.c_uint64
        cls.lib.streamoo_canvas_init.argtypes = [C.POINTER(Canvas), C.POINTER(C.c_uint8), C.c_size_t,
                                                C.c_int16, C.c_int16, C.c_uint16]
        cls.lib.streamoo_text_line.argtypes = [C.POINTER(Canvas), Rect, C.c_char_p, C.c_size_t,
                                              C.c_int, C.c_uint64, C.c_uint32, C.c_uint32,
                                              C.c_uint8, GLYPH, C.c_void_p]
        cls.lib.streamoo_text_line.restype = C.c_bool
        cls.lib.streamoo_text_draw_scrolled.argtypes = [C.POINTER(Canvas), C.c_int32, C.c_int32,
                                                       C.c_uint64, C.c_char_p, C.c_size_t,
                                                       C.c_uint8, GLYPH, C.c_void_p]
        cls.lib.streamoo_text_layout_smoke.argtypes = [C.POINTER(C.c_uint8), C.c_size_t, C.c_uint64]

    def setUp(self):
        self.guard = (C.c_uint8 * 514)()
        self.guard[0] = self.guard[-1] = 0xa5
        self.pointer = C.cast(C.byref(self.guard, 1), C.POINTER(C.c_uint8))
        self.canvas = Canvas()
        self.lib.streamoo_canvas_init(C.byref(self.canvas), self.pointer, 512, 64, 16, 32)

        def glyph(context, code, columns):
            for index in range(5):
                columns[index] = 0xff
            return True
        self.glyph = GLYPH(glyph)

    def pixel(self, x, y=0):
        return (self.guard[1 + y * 32 + x // 2] >> (0 if x & 1 else 4)) & 15

    def test_pause_left_motion_and_repeat_cycle(self):
        expected = {0: 0, 999: 0, 1000: 0, 1500: 15, 2999: 59,
                    3000: 60, 3999: 89, 4000: 90, 4500: 105, 5133: 123, 5134: 0}
        for time, offset in expected.items():
            self.assertEqual(self.lib.streamoo_marquee_offset(100, 40, time, 30, 1000), offset)

    def test_fit_disabled_and_uint_boundaries(self):
        function = self.lib.streamoo_marquee_offset
        for args in ((40, 40, 999, 30, 1000), (30, 40, 999, 30, 1000),
                     (100, 40, 999, 0, 1000), (100, 0, 999, 30, 1000)):
            self.assertEqual(function(*args), 0)
        for speed in (1, 30, 0xffffffff):
            self.assertLessEqual(function(0xffffffff, 1, 0xffffffffffffffff, speed, 0xffffffff), 0xffffffff + 23)

    def test_left_center_right_and_vertical_clipping(self):
        for align, start in ((0, 10), (1, 17), (2, 25)):
            for i in range(1, 513):
                self.guard[i] = 0
            self.assertTrue(self.lib.streamoo_text_line(C.byref(self.canvas), Rect(10, 0, 20, 4),
                                                        b'A', 1, align, 0, 30, 1000, 15, self.glyph, None))
            self.assertEqual([x for x in range(64) if self.pixel(x)], list(range(start, start + 5)))
            self.assertTrue(all(self.pixel(x, 4) == 0 for x in range(64)))
        self.assertEqual((self.guard[0], self.guard[-1]), (0xa5, 0xa5))

    def test_long_offset_has_no_int16_wrap(self):
        data = b'A' * 14000
        self.assertTrue(self.lib.streamoo_text_draw_scrolled(C.byref(self.canvas), 0, 0,
                                                            80000, data, len(data), 15, self.glyph, None))
        self.assertEqual([self.pixel(x) for x in range(64)],
                         [0 if (80000 + x) % 6 == 5 else 15 for x in range(64)])
        self.assertEqual((self.guard[0], self.guard[-1]), (0xa5, 0xa5))

    def test_frame_skips_and_reset_are_deterministic(self):
        function = self.lib.streamoo_marquee_offset
        result = function(100, 40, 2345, 30, 1000)
        for time in range(0, 2345, 40):
            function(100, 40, time, 30, 1000)
        self.assertEqual(function(100, 40, 2345, 30, 1000), result)
        self.assertEqual(function(100, 40, 0, 30, 1000), 0)

    def test_diagnostic_header_static_and_metadata_scrolls(self):
        frame = (C.c_uint8 * 8192)()
        self.assertTrue(self.lib.streamoo_text_layout_smoke(frame, 8192, 0))
        initial = bytes(frame)
        self.assertTrue(self.lib.streamoo_text_layout_smoke(frame, 8192, 2500))
        self.assertEqual(bytes(frame)[:16 * 128], initial[:16 * 128])
        self.assertNotEqual(bytes(frame)[16 * 128:32 * 128], initial[16 * 128:32 * 128])

    def test_repeat_enters_from_right_after_gap(self):
        data = b'A' * 8  # nominal ink 47px + 24px gap = 71px period
        self.assertTrue(self.lib.streamoo_text_line(C.byref(self.canvas), Rect(10, 0, 20, 4),
                                                    data, len(data), 0, 3000, 30, 1000,
                                                    15, self.glyph, None))
        expected = []
        for x in range(64):
            source = (60 + x - 10) % 71
            expected.append(15 if 10 <= x < 30 and source < 47 and source % 6 < 5 else 0)
        self.assertEqual([self.pixel(x) for x in range(64)], expected)
        self.assertEqual((self.guard[0], self.guard[-1]), (0xa5, 0xa5))

    def test_offset_never_reverses_between_wraps(self):
        values = [self.lib.streamoo_marquee_offset(100, 40, t, 30, 1000)
                  for t in range(1000, 5134)]
        self.assertEqual(values, sorted(values))
        self.assertEqual(max(values), 123)

    def test_excessive_public_scroll_offset_is_rejected(self):
        before = bytes(self.guard)
        self.assertFalse(self.lib.streamoo_text_draw_scrolled(C.byref(self.canvas), 0, 0,
                                                             0xffffffffffffffff, b'A', 1,
                                                             15, self.glyph, None))
        self.assertEqual(bytes(self.guard), before)
