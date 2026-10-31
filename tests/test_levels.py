"""Actual stereo RMS/peak analyzer with independent signal references and PCM tap."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
class LevelsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        cls.binary = str(Path(cls.temp.name) / 'levels')
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
            *os.environ.get('STREAMOO_LEVELS_SANITIZE', '').split(),
            '-Icomponents/levels/include', '-Icomponents/pcm/include',
            '-Icomponents/audio_gain/include', 'components/levels/streamoo_levels.c',
            'components/pcm/streamoo_pcm.c', 'components/audio_gain/streamoo_audio_gain.c',
            'tests/levels_test.c', '-lm', '-o', cls.binary], cwd=ROOT, check=True)
    def case(self, n):
        subprocess.run([self.binary, str(n)], check=True)
    def test_invalid_calls_are_atomic(self): self.case(0)
    def test_silence_waits_for_complete_window(self): self.case(1)
    def test_full_scale_longest_window_no_overflow(self): self.case(2)
    def test_stereo_sines_100_1000_8000_hz(self): self.case(3)
    def test_impulse_and_window_reset(self): self.case(4)
    def test_gap_duplicate_and_session_reset(self): self.case(5)
    def test_chunk_boundaries_and_sequence_wrap(self): self.case(6)
    def test_actual_pcm_tap_at_volume_0_50_100(self): self.case(7)
    def test_sub_q15_quantization(self): self.case(8)
