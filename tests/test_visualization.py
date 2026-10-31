import ctypes as C
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_canvas import Canvas, Rect

ROOT = Path(__file__).resolve().parents[1]


class Model(C.Structure):
    _fields_ = [('rms', C.c_uint16 * 2), ('peak', C.c_uint16 * 2),
                ('bands', C.c_uint16 * 64), ('band_count', C.c_uint8)]


class VisualizationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        library = Path(cls.temp.name) / 'visualization.so'
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC',
                        '-Icomponents/graphics/include', '-Icomponents/ui_widgets/include',
                        'components/graphics/streamoo_canvas.c',
                        'components/ui_widgets/streamoo_visualization.c', '-o', str(library)],
                       cwd=ROOT, check=True)
        cls.lib = C.CDLL(str(library))
        cls.lib.streamoo_canvas_init.argtypes = [C.POINTER(Canvas), C.POINTER(C.c_uint8), C.c_size_t,
                                                C.c_int16, C.c_int16, C.c_uint16]
        cls.lib.streamoo_canvas_clip.argtypes = [C.POINTER(Canvas), Rect]
        cls.lib.streamoo_visualization_draw.argtypes = [C.POINTER(Canvas), Rect, C.c_int, C.POINTER(Model)]
        cls.lib.streamoo_visualization_draw.restype = C.c_bool

    def setUp(self):
        self.guard = (C.c_uint8 * 8194)(*([0xa5] * 8194))
        self.pointer = C.cast(C.byref(self.guard, 1), C.POINTER(C.c_uint8))
        self.canvas = Canvas()
        self.lib.streamoo_canvas_init(C.byref(self.canvas), self.pointer, 8192, 256, 64, 128)
        self.bounds = Rect(44, 32, 132, 32)
        self.model = Model()
        self.model.band_count = 24
        for i in range(24):
            self.model.bands[i] = i * 32767 // 23
        self.model.rms[0], self.model.rms[1] = 8192, 24576
        self.model.peak[0], self.model.peak[1] = 16384, 32767

    def pixel(self, x, y):
        return (self.guard[1 + y * 128 + x // 2] >> (0 if x & 1 else 4)) & 15

    def draw(self, kind):
        return self.lib.streamoo_visualization_draw(C.byref(self.canvas), self.bounds,
                                                    kind, C.byref(self.model))

    def assert_outside_unchanged(self, rect):
        self.assertEqual((self.guard[0], self.guard[-1]), (0xa5, 0xa5))
        for y in range(64):
            for x in range(256):
                if not (rect.x <= x < rect.x + rect.width and rect.y <= y < rect.y + rect.height):
                    self.assertEqual(self.pixel(x, y), 5 if x & 1 else 10)

    def test_all_modes_respect_bounds_and_do_not_mutate_snapshot(self):
        for kind in range(3):
            self.setUp()
            before = bytes(self.model)
            self.assertTrue(self.draw(kind))
            self.assertEqual(bytes(self.model), before)
            self.assert_outside_unchanged(self.bounds)
            self.assertNotEqual(bytes(self.guard[1:-1]), b'\xa5' * 8192)

    def test_parent_clip_is_inherited(self):
        clip = Rect(80, 40, 20, 10)
        for kind in range(3):
            self.setUp()
            self.lib.streamoo_canvas_clip(C.byref(self.canvas), clip)
            self.assertTrue(self.draw(kind))
            self.assert_outside_unchanged(clip)

    def test_invalid_inputs_fail_before_writes(self):
        for kind, bounds, field in ((-1, self.bounds, None), (3, self.bounds, None),
                                    (0, Rect(-1, 32, 132, 32), None),
                                    (0, Rect(200, 32, 132, 32), None),
                                    (0, Rect(44, 32, 7, 32), None),
                                    (0, self.bounds, 'rms'), (0, self.bounds, 'peak'),
                                    (0, self.bounds, 'bands'), (0, self.bounds, 'count')):
            self.setUp()
            if field in ('rms', 'peak', 'bands'):
                getattr(self.model, field)[0] = 65535
            if field == 'count':
                self.model.band_count = 65
            self.assertFalse(self.lib.streamoo_visualization_draw(C.byref(self.canvas), bounds,
                                                                  kind, C.byref(self.model)))
            self.assertTrue(all(value == 0xa5 for value in self.guard))

    def test_spectrum_full_scale_and_empty_snapshot(self):
        self.model = Model()
        self.model.band_count = 24
        self.model.bands[0] = 32767
        self.assertTrue(self.draw(0))
        self.assertEqual(self.pixel(45, 32), 15)
        self.assertEqual(self.pixel(45, 33), 8)
        self.assertEqual(self.pixel(60, 32), 0)
        self.assertEqual(self.pixel(45, 63), 4)
        self.model.band_count = 0
        self.assertTrue(self.draw(0))
        self.assertEqual(self.pixel(45, 32), 0)
        self.assertEqual(self.pixel(45, 63), 4)

    def test_stereo_bargraph_keeps_channels_independent(self):
        self.model = Model()
        self.model.rms[0] = self.model.peak[0] = 32767
        self.assertTrue(self.draw(1))
        self.assertEqual(self.pixel(50, 40), 10)
        self.assertEqual(self.pixel(175, 40), 15)
        self.assertEqual(self.pixel(50, 56), 0)
        self.model.rms[0] = self.model.peak[0] = 0
        self.model.rms[1] = 32767
        self.assertTrue(self.draw(1))
        self.assertEqual(self.pixel(50, 40), 0)  # previous mode data are cleared
        self.assertEqual(self.pixel(50, 56), 10)

    def test_analog_zero_and_full_scale_needle_positions(self):
        self.model = Model()
        self.model.rms[1] = 32767
        self.assertTrue(self.draw(2))
        self.assertEqual(self.pixel(50, 48), 15)  # left channel low
        self.assertEqual(self.pixel(170, 48), 15)  # right channel high
        self.assertEqual(self.pixel(77, 63), 15)
        self.assertEqual(self.pixel(143, 63), 15)

    def test_minimum_odd_and_full_size_bounds(self):
        for bounds in (Rect(0, 0, 8, 8), Rect(1, 1, 133, 33), Rect(0, 0, 256, 64)):
            for kind in range(3):
                self.setUp()
                self.bounds = bounds
                self.model.band_count = 64
                for i in range(64):
                    self.model.bands[i] = 32767
                self.assertTrue(self.draw(kind))
                self.assert_outside_unchanged(bounds)
