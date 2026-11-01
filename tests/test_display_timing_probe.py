"""Check the optical experiment's controls using its actual C renderer."""
import ctypes as C
from collections import Counter
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def gray_counts(frame):
    return Counter(gray for byte in frame for gray in (byte >> 4, byte & 15))


class DisplayTimingProbeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        output = Path(cls.temp.name) / "probe.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-shared", "-fPIC",
                        "-I", str(ROOT / "main"), "-I", str(ROOT / "components/graphics/include"),
                        "-I", str(ROOT / "components/text/include"),
                        *[str(ROOT / source) for source in (
                            "main/display_timing_probe.c", "components/graphics/streamoo_canvas.c",
                            "components/text/streamoo_text.c", "components/text/streamoo_font5x8.c")],
                        "-o", str(output)], check=True)
        cls.lib = C.CDLL(str(output))
        cls.lib.streamoo_oled_probe_draw.argtypes = [C.POINTER(C.c_uint8), C.c_size_t, C.c_int, C.c_uint32]
        cls.lib.streamoo_oled_probe_draw.restype = C.c_bool

    def render(self, phase, elapsed):
        storage = (C.c_uint8 * 8194)(*([0xa5] * 8194))
        frame = C.cast(C.byref(storage, 1), C.POINTER(C.c_uint8))
        self.assertTrue(self.lib.streamoo_oled_probe_draw(frame, 8192, phase, elapsed))
        self.assertEqual((storage[0], storage[-1]), (0xa5, 0xa5))
        return bytes(storage[1:-1])

    def test_held_and_retransmitted_controls_are_identical(self):
        held = self.render(0, 0)
        for time in (0, 40, 1000, 7999):
            self.assertEqual(self.render(0, time), held)
            self.assertEqual(self.render(1, time), held)

    def test_reference_area_is_constant_through_all_motion(self):
        reference = self.render(0, 0)[:32 * 128]
        for phase in range(5):
            for time in range(0, 2040, 40):
                self.assertEqual(self.render(phase, time)[:32 * 128], reference)

    def test_load_animation_matches_between_rates_but_changes_lit_area(self):
        for time in (0, 100, 600, 1000, 1600):
            self.assertEqual(self.render(2, time), self.render(3, time))
        self.assertNotEqual(gray_counts(self.render(2, 0)), gray_counts(self.render(2, 1000)))

    def test_moving_control_preserves_pixel_count_at_every_gray(self):
        initial = self.render(4, 0)
        self.assertNotEqual(self.render(4, 1000), initial)
        for time in range(0, 2040, 40):
            self.assertEqual(gray_counts(self.render(4, time)), gray_counts(initial))

    def test_invalid_input_leaves_memory_untouched(self):
        frame = (C.c_uint8 * 8192)(*([0xa5] * 8192))
        for phase, size in ((-1, 8192), (5, 8192), (0, 8191)):
            self.assertFalse(self.lib.streamoo_oled_probe_draw(frame, size, phase, 0))
            self.assertEqual(bytes(frame), bytes([0xa5]) * 8192)
