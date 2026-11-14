from pathlib import Path
import os,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class SpectrumStreamTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup)
  cls.binary=str(Path(cls.tmp.name)/'collector')
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',*os.environ.get('STREAMOO_SPECTRUM_STREAM_SANITIZE','').split(),'-Icomponents/spectrum/include','components/spectrum/streamoo_spectrum.c','components/spectrum/streamoo_spectrum_stream.c','tests/spectrum_stream_test.c','-lm','-o',cls.binary],cwd=ROOT,check=True)
 def case(self,n):subprocess.run([self.binary,str(n)],check=True)
 def test_invalid_calls_do_not_fill(self):self.case(0)
 def test_overlap_matches_independent_complete_frames(self):self.case(1)
 def test_gap_discards_partial_window(self):self.case(2)
 def test_skip_preserves_overlap_without_fft(self):self.case(3)
 def test_db_scale_monotonic_and_reference_points(self):self.case(4)
 def test_sequence_rollover_and_selection_reset(self):self.case(5)
 def test_shared_scale_is_monotonic_bounded_and_less_sensitive(self):self.case(6)
