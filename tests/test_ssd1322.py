"""Execute the firmware protocol against a recording/failing SPI transport."""
import ctypes as C
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WRITE = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_bool, C.POINTER(C.c_uint8), C.c_size_t)


class Profile(C.Structure):
    _fields_ = [(name, C.c_uint8) for name in
                ('column_offset', 'remap', 'contrast', 'master_current')]


class Display(C.Structure):
    _fields_ = [('write', WRITE), ('context', C.c_void_p),
                ('profile', Profile), ('ready', C.c_bool)]


class SSD1322Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        output = Path(cls.temp.name) / 'ssd1322.so'
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC',
                        '-I', str(ROOT / 'components/ssd1322/include'),
                        str(ROOT / 'components/ssd1322/ssd1322_protocol.c'),
                        '-o', str(output)], check=True)
        cls.lib = C.CDLL(str(output))
        cls.lib.ssd1322_protocol_init.argtypes = [C.POINTER(Display), WRITE, C.c_void_p,
                                                 C.POINTER(Profile)]
        cls.lib.ssd1322_protocol_present.argtypes = [C.POINTER(Display), C.POINTER(C.c_uint8),
                                                    C.c_size_t]
        cls.lib.ssd1322_protocol_enable.argtypes = [C.POINTER(Display), C.c_bool]
        cls.lib.ssd1322_test_pattern.argtypes = [C.POINTER(C.c_uint8), C.c_size_t]

    def setUp(self):
        self.display = Display()
        self.profile = Profile(28, 0x14, 0x7f, 3)
        self.events = []
        self.fail_at = -1
        self.fail_code = 0x107

        def write(context, data, pointer, length):
            self.events.append((data, C.string_at(pointer, length)))
            return self.fail_code if len(self.events) - 1 == self.fail_at else 0
        self.write = WRITE(write)
        self.frame = (C.c_uint8 * 8192)()

    def initialize(self):
        return self.lib.ssd1322_protocol_init(C.byref(self.display), self.write,
                                             None, C.byref(self.profile))

    def test_initialization_commands_and_dc(self):
        self.assertEqual(self.initialize(), 0)
        self.assertTrue(self.display.ready)
        commands = [value for data, value in self.events if not data]
        self.assertEqual(commands, [bytes([value]) for value in
                         (0xfd, 0xae, 0xa0, 0xca, 0xa1, 0xa2, 0xa9, 0xab, 0xb3,
                          0xb1, 0xb4, 0xb6, 0xbb, 0xbe, 0xc1, 0xc7, 0xd1, 0xb9, 0xa6)])
        # Parameters must be DATA, not extra command bytes. Display stays OFF.
        self.assertEqual(self.events[:7], [(False, b'\xfd'), (True, b'\x12'),
                         (False, b'\xae'), (False, b'\xa0'), (True, b'\x14\x11'),
                         (False, b'\xca'), (True, b'\x3f')])
        self.assertNotIn((False, b'\xaf'), self.events)
        vsl = self.events.index((False, b'\xb4'))
        self.assertEqual(self.events[vsl + 1], (True, b'\xa0\xb5'))
        clock = self.events.index((False, b'\xb3'))
        phase = self.events.index((False, b'\xb1'))
        self.assertEqual(self.events[clock + 1], (True, b'\x91'))
        self.assertEqual(self.events[phase + 1], (True, b'\xe2'))

    def test_negative_transport_error_is_preserved(self):
        self.fail_at = 0
        self.fail_code = -1
        self.assertEqual(self.initialize(), -1)
        self.assertFalse(self.display.ready)

    def test_all_initialization_transport_failures_stop_immediately(self):
        self.assertEqual(self.initialize(), 0)
        write_count = len(self.events)
        for index in range(write_count):
            with self.subTest(write=index):
                self.events = []
                self.fail_at = index
                self.assertEqual(self.initialize(), 0x107)
                self.assertFalse(self.display.ready)
                self.assertEqual(len(self.events), index + 1)

    def test_exact_frame_window_and_bytes(self):
        self.assertEqual(self.initialize(), 0)
        self.events = []
        self.frame[0], self.frame[-1] = 0x12, 0xef
        self.assertEqual(self.lib.ssd1322_protocol_present(C.byref(self.display), self.frame, 8192), 0)
        self.assertEqual(self.events[:5], [(False, b'\x15'), (True, b'\x1c\x5b'),
                         (False, b'\x75'), (True, b'\x00\x3f'), (False, b'\x5c')])
        self.assertEqual(self.events[5], (True, bytes(self.frame)))

    def test_all_frame_transport_failures_propagate(self):
        self.assertEqual(self.initialize(), 0)
        for index in range(6):
            self.events = []
            self.fail_at = index
            self.assertEqual(self.lib.ssd1322_protocol_present(C.byref(self.display), self.frame, 8192), 0x107)
            self.assertEqual(len(self.events), index + 1)

    def test_invalid_frames_and_uninitialized_access_do_not_write(self):
        self.assertEqual(self.lib.ssd1322_protocol_enable(C.byref(self.display), True), -1)
        self.assertEqual(self.lib.ssd1322_protocol_present(C.byref(self.display), self.frame, 8192), -1)
        self.assertEqual(self.events, [])
        self.initialize()
        self.events = []
        for length in (0, 1, 8191, 8193):
            self.assertEqual(self.lib.ssd1322_protocol_present(C.byref(self.display), self.frame, length), -1)
        self.assertEqual(self.lib.ssd1322_protocol_present(C.byref(self.display), None, 8192), -1)
        self.assertEqual(self.events, [])

    def test_profile_validation_and_maximum_column_window(self):
        for profile in (Profile(57, 20, 127, 3), Profile(28, 1, 127, 3),
                        Profile(28, 32, 127, 3), Profile(28, 20, 127, 16)):
            self.assertEqual(self.lib.ssd1322_protocol_init(C.byref(self.display), self.write,
                                                           None, C.byref(profile)), -1)
            self.assertEqual(self.events, [])
        self.profile.column_offset = 56
        self.assertEqual(self.initialize(), 0)
        self.events = []
        self.assertEqual(self.lib.ssd1322_protocol_present(C.byref(self.display), self.frame, 8192), 0)
        self.assertEqual(self.events[1], (True, b'\x38\x77'))

    def test_enable_disable_and_error(self):
        self.initialize()
        self.events = []
        self.assertEqual(self.lib.ssd1322_protocol_enable(C.byref(self.display), True), 0)
        self.assertEqual(self.lib.ssd1322_protocol_enable(C.byref(self.display), False), 0)
        self.assertEqual(self.events, [(False, b'\xaf'), (False, b'\xae')])
        self.fail_at = 2
        self.assertEqual(self.lib.ssd1322_protocol_enable(C.byref(self.display), False), 0x107)

    def test_pattern_4bpp_bounds_orientation_and_gray_levels(self):
        guard = (C.c_uint8 * 8194)(*([0xa5] * 8194))
        pointer = C.cast(C.byref(guard, 1), C.POINTER(C.c_uint8))
        self.assertEqual(self.lib.ssd1322_test_pattern(pointer, 8192), 0)
        self.assertEqual((guard[0], guard[-1]), (0xa5, 0xa5))
        frame = bytes(guard[1:-1])
        self.assertEqual(frame[:8], bytes.fromhex('01 23 45 67 89 ab cd ef'))
        # Mid-screen has exactly 16 bands and distinct left/right boundary pixels.
        row = frame[32 * 128:33 * 128]
        self.assertEqual(row[:2], b'\xf0\x00')
        for level in range(1, 15):
            self.assertEqual(row[level * 8:(level + 1) * 8], bytes([level * 17]) * 8)
        self.assertEqual(row[-8:], b'\xff' * 8)
        self.assertEqual(frame[-128:], b'\xff' * 128)
        # Reject incorrect size without touching caller memory.
        before = bytes(guard)
        self.assertEqual(self.lib.ssd1322_test_pattern(pointer, 8191), -1)
        self.assertEqual(bytes(guard), before)
