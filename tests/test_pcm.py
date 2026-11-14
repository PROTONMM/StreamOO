from pathlib import Path
import os
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class PCMTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory(); cls.addClassCleanup(cls.temp.cleanup)
        cls.binary=str(Path(cls.temp.name)/'pcm')
        flags=os.environ.get('STREAMOO_PCM_SANITIZE','').split()
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-pthread',*flags,
            '-Icomponents/pcm/include','-Icomponents/audio_gain/include',
            'components/pcm/streamoo_pcm.c','components/audio_gain/streamoo_audio_gain.c',
            'tests/pcm_test.c','-o',cls.binary],cwd=ROOT,check=True)
    def case(self,n): subprocess.run([self.binary,str(n)],check=True)
    def test_initialization_contract(self): self.case(0)
    def test_fifo_full_empty_and_no_overwrite(self): self.case(1)
    def test_counter_wrap(self): self.case(2)
    def test_threaded_spsc_20000_blocks(self): self.case(3)
    def test_tap_before_gain_0_50_100(self): self.case(4)
    def test_reserve_fade_and_rebuffer(self): self.case(5)
    def test_stop_consumes_source(self): self.case(6)
    def test_slow_tap_drops_without_audio_effect(self): self.case(7)
    def test_invalid_fill_atomic(self): self.case(8)
    def test_startup_threshold(self): self.case(9)
    def test_unexpected_empty_fades_and_tap_is_zero(self): self.case(10)
