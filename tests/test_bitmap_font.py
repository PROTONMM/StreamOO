import ctypes as C
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_canvas import Canvas, Rect

ROOT = Path(__file__).resolve().parents[1]


class Glyph(C.Structure):
    _fields_ = [('cp', C.c_uint32), ('offset', C.c_uint32), ('x', C.c_int16),
                ('y', C.c_int16), ('advance', C.c_int16), ('w', C.c_uint16), ('h', C.c_uint16)]


class Font(C.Structure):
    _fields_ = [('glyphs', C.POINTER(Glyph)), ('bitmap', C.POINTER(C.c_uint8)),
                ('count', C.c_uint32), ('bitmap_bytes', C.c_uint32),
                ('line_height', C.c_uint16), ('ascent', C.c_int16)]


class BitmapFontTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        library = Path(cls.temp.name) / 'font.so'
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC',
                        '-Icomponents/graphics/include', '-Icomponents/text/include', '-Imain',
                        'components/graphics/streamoo_canvas.c', 'components/text/streamoo_text.c',
                        'components/text/streamoo_text_layout.c', 'components/text/streamoo_font5x8.c',
                        'components/text/streamoo_bitmap_font.c', 'components/text/streamoo_ui_font.c',
                        'main/text_layout_smoke.c', '-o', str(library)], cwd=ROOT, check=True)
        cls.lib = C.CDLL(str(library))
        cls.lib.streamoo_ui_font.restype = C.POINTER(Font)
        cls.lib.streamoo_font_lookup.argtypes = [C.POINTER(Font), C.c_uint32]
        cls.lib.streamoo_font_lookup.restype = C.POINTER(Glyph)
        cls.lib.streamoo_bitmap_text_width.argtypes = [C.POINTER(Font), C.c_char_p, C.c_size_t]
        cls.lib.streamoo_bitmap_text_width.restype = C.c_size_t
        cls.lib.streamoo_bitmap_text_line.argtypes = [C.POINTER(Canvas), Rect, C.POINTER(Font),
                                                      C.c_char_p, C.c_size_t, C.c_int, C.c_uint64,
                                                      C.c_uint32, C.c_uint32, C.c_uint8]
        cls.lib.streamoo_bitmap_text_line.restype = C.c_bool
        cls.lib.streamoo_canvas_init.argtypes = [C.POINTER(Canvas), C.POINTER(C.c_uint8), C.c_size_t,
                                                 C.c_int16, C.c_int16, C.c_uint16]
        cls.lib.streamoo_canvas_alpha4.argtypes = [C.POINTER(Canvas)] + [C.c_int16] * 4 + [
            C.POINTER(C.c_uint8), C.c_size_t, C.c_uint16, C.c_uint8]
        cls.lib.streamoo_canvas_alpha4.restype = C.c_bool
        cls.lib.streamoo_text_layout_smoke.argtypes = [C.POINTER(C.c_uint8), C.c_size_t, C.c_uint64]

    def test_coverage_metrics_and_bitmap_spans(self):
        font = self.lib.streamoo_ui_font().contents
        self.assertEqual((font.count, font.line_height, font.ascent), (113, 16, 13))
        wanted = sorted(list(range(32, 127)) + list(map(ord, 'ąćęłńóśźżĄĆĘŁŃÓŚŹŻ')))
        for i, cp in enumerate(wanted):
            glyph = font.glyphs[i]
            self.assertEqual(glyph.cp, cp)
            self.assertLessEqual(glyph.offset + ((glyph.w + 1) // 2) * glyph.h, font.bitmap_bytes)
            self.assertGreaterEqual(glyph.y + font.ascent, 0)
            self.assertLessEqual(glyph.y + font.ascent + glyph.h, 16)
            self.assertGreater(glyph.advance, 0)
            if glyph.w % 2:
                for row in range(glyph.h):
                    self.assertEqual(font.bitmap[glyph.offset + (row + 1) * ((glyph.w + 1) // 2) - 1] & 15, 0)

    def test_measure_unicode_and_fallback(self):
        font = self.lib.streamoo_ui_font()
        text = 'ĄĆĘŁŃÓŚŹŻ'.encode()
        self.assertEqual(self.lib.streamoo_bitmap_text_width(font, text, len(text)), 9 * 8)
        self.assertEqual(self.lib.streamoo_bitmap_text_width(font, '🙂'.encode(), 4),
                         self.lib.streamoo_bitmap_text_width(font, b'?', 1))
        self.assertFalse(self.lib.streamoo_font_lookup(font, 0x1f642))

    def test_alpha_composition_odd_rows_and_padding(self):
        buffer = (C.c_uint8 * 10)(*([0x55] * 10))
        pointer = C.cast(C.byref(buffer, 1), C.POINTER(C.c_uint8))
        canvas = Canvas()
        self.lib.streamoo_canvas_init(C.byref(canvas), pointer, 8, 5, 2, 4)
        mask = (C.c_uint8 * 4)(0x08, 0xf0, 0xf8, 0x00)
        self.assertTrue(self.lib.streamoo_canvas_alpha4(C.byref(canvas), 1, 0, 3, 2, mask, 4, 2, 15))
        self.assertEqual(bytes(buffer), bytes.fromhex('55 55 af 55 55 5f a5 55 55 55'))
        before = bytes(buffer)
        self.assertFalse(self.lib.streamoo_canvas_alpha4(C.byref(canvas), 0, 0, 3, 2, mask, 3, 2, 15))
        self.assertEqual(bytes(buffer), before)

    def test_invalid_glyph_span_rejected_before_drawing(self):
        font = self.lib.streamoo_ui_font().contents
        glyph = Glyph(ord('A'), font.bitmap_bytes, 0, -1, 8, 8, 8)
        bad = Font(C.pointer(glyph), font.bitmap, 1, font.bitmap_bytes, 16, 13)
        frame = (C.c_uint8 * 8192)(*([0xa5] * 8192))
        canvas = Canvas()
        self.lib.streamoo_canvas_init(C.byref(canvas), frame, 8192, 256, 64, 128)
        self.assertEqual(self.lib.streamoo_bitmap_text_width(C.byref(bad), b'A', 1), C.c_size_t(-1).value)
        self.assertFalse(self.lib.streamoo_bitmap_text_line(C.byref(canvas), Rect(0, 0, 256, 16),
                                                            C.byref(bad), b'A', 1, 1, 0, 30, 1000, 15))
        self.assertEqual(bytes(frame), b'\xa5' * 8192)

    def test_invalid_base_font_is_rejected_for_empty_text(self):
        bad = Font(None, None, 0, 0, 16, 13)
        self.assertEqual(self.lib.streamoo_bitmap_text_width(C.byref(bad), b'', 0), C.c_size_t(-1).value)

    def test_cyclic_alpha_font_repeat_moves_only_left(self):
        glyph = Glyph(ord('A'), 0, 0, 0, 2, 1, 1)
        bitmap = (C.c_uint8 * 1)(0xf0)
        font = Font(C.pointer(glyph), bitmap, 1, 1, 1, 0)
        frame = (C.c_uint8 * 8)()
        canvas = Canvas()
        self.lib.streamoo_canvas_init(C.byref(canvas), frame, 8, 16, 1, 8)
        for time, expected in ((3000, [4, 6, 8]), (3034, [3, 5, 7, 9]),
                               (3133, [1, 3, 5, 7, 9]), (3134, [0, 2, 4, 6, 8])):
            for i in range(8):
                frame[i] = 0
            self.assertTrue(self.lib.streamoo_bitmap_text_line(C.byref(canvas), Rect(0, 0, 10, 1),
                                                               C.byref(font), b'A' * 20, 20,
                                                               0, time, 30, 1000, 15))
            active = [x for x in range(16) if ((frame[x // 2] >> (0 if x & 1 else 4)) & 15)]
            self.assertEqual(active, expected)

    def test_layout_matches_frozen_frame_and_cannot_write_outside_buffer(self):
        guard = (C.c_uint8 * 8194)(*([0xa5] * 8194))
        pointer = C.cast(C.byref(guard, 1), C.POINTER(C.c_uint8))
        self.assertTrue(self.lib.streamoo_text_layout_smoke(pointer, 8192, 0))
        self.assertEqual(bytes(guard[1:-1]), (ROOT / 'tests/fixtures/krypton_layout.4bpp').read_bytes())
        self.assertEqual((guard[0], guard[-1]), (0xa5, 0xa5))


class FontGeneratorTests(unittest.TestCase):
    def setUp(self):
        try:
            import PIL
            import fontTools
        except ImportError:
            self.skipTest('Optional host font-generation dependencies not installed')
        spec = importlib.util.spec_from_file_location('fontgen', ROOT / 'tools/font_generator/ttf_to_ssd1322.py')
        self.module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.module)

    def test_odd_width_rows_do_not_share_nibbles(self):
        from PIL import Image
        image = Image.new('L', (3, 2))
        image.putdata([255, 0, 128, 64, 255, 0])
        self.assertEqual(bytes(self.module.pack_4bpp(image)), bytes.fromhex('f0 80 4f 00'))

    def test_unicode_ranges_reject_surrogates_and_invalid_scalar(self):
        for value in ('D800', '110000', '007E-0020'):
            with self.assertRaises(ValueError):
                self.module.codepoints(value, '')
        self.assertEqual(self.module.codepoints('0104,U+017B', 'Ą'), [0x104, 0x17b])

    def test_generator_rejects_missing_source_glyph(self):
        from types import SimpleNamespace
        with tempfile.TemporaryDirectory() as directory:
            args = SimpleNamespace(ttf=ROOT / 'assets/fonts/MonaspaceKrypton-Regular.otf',
                                   size=13, chars_file=None, ranges='10FFFF',
                                   out=Path(directory) / 'not_written.h', name='streamoo_ui_16',
                                   preview=None, report=None)
            with self.assertRaisesRegex(ValueError, 'Missing source glyphs'):
                self.module.emit(args)
            self.assertFalse(args.out.exists())
