import ctypes as C
import hashlib
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
POLISH = 'ąćęłńóśźżĄĆĘŁŃÓŚŹŻ'


class FontTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        library = Path(cls.temp.name) / 'font.so'
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC',
                        '-Icomponents/graphics/include', '-Icomponents/text/include', '-Imain',
                        'components/graphics/streamoo_canvas.c', 'components/text/streamoo_text.c',
                        'components/text/streamoo_font5x8.c', 'main/text_smoke.c', '-o', str(library)],
                       cwd=ROOT, check=True)
        cls.lib = C.CDLL(str(library))
        cls.lib.streamoo_font5x8.argtypes = [C.c_void_p, C.c_uint32, C.POINTER(C.c_uint8)]
        cls.lib.streamoo_font5x8.restype = C.c_bool
        cls.lib.streamoo_text_smoke.argtypes = [C.POINTER(C.c_uint8), C.c_size_t]
        cls.lib.streamoo_text_smoke.restype = C.c_bool

    def test_all_ascii_and_polish_bitmaps_match_source_subset_digest(self):
        # Digest independently computed from the reviewed ZIP: Unicode LE32 + 5 columns.
        digest = hashlib.sha256()
        for code in sorted(list(range(32, 127)) + list(map(ord, POLISH))):
            columns = (C.c_uint8 * 5)()
            self.assertTrue(self.lib.streamoo_font5x8(None, code, columns))
            digest.update(code.to_bytes(4, 'little') + bytes(columns))
        self.assertEqual(digest.hexdigest(), '8cf10b08809d64d31b93ef9b179355256ae50b2d686c399f87666af880e3ae53')

    def test_unknown_unicode_is_not_legacy_byte_index(self):
        columns = (C.c_uint8 * 5)(*[0xa5] * 5)
        for code in (0, 31, 127, 0xb8, 0xd6, 0xfffd, 0x1f642):
            self.assertFalse(self.lib.streamoo_font5x8(None, code, columns))
            self.assertEqual(bytes(columns), b'\xa5' * 5)
        self.assertFalse(self.lib.streamoo_font5x8(None, ord('ą'), None))

    def test_ogonek_bit_is_retained(self):
        for char in 'ąĄęĘ':
            columns = (C.c_uint8 * 5)()
            self.assertTrue(self.lib.streamoo_font5x8(None, ord(char), columns))
            self.assertTrue(any(value & 0x80 for value in columns))

    def test_complete_text_scene_and_buffer_bounds(self):
        guard = (C.c_uint8 * 8194)(*([0xa5] * 8194))
        pointer = C.cast(C.byref(guard, 1), C.POINTER(C.c_uint8))
        self.assertTrue(self.lib.streamoo_text_smoke(pointer, 8192))
        self.assertEqual((guard[0], guard[-1]), (0xa5, 0xa5))
        self.assertEqual(bytes(guard[1:-1]), (ROOT / 'tests/fixtures/polish_text.4bpp').read_bytes())
        self.assertTrue(any(guard[1 + 63 * 128:-1]))  # bottom row of ogonek test
        before = bytes(guard)
        self.assertFalse(self.lib.streamoo_text_smoke(pointer, 8191))
        self.assertEqual(bytes(guard), before)
