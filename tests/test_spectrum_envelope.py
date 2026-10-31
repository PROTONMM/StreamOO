from pathlib import Path
import os,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class SpectrumEnvelopeTests(unittest.TestCase):
 def test_attack_immediate_release_silence_and_reset(self):
  with tempfile.TemporaryDirectory() as tmp:
   binary=str(Path(tmp)/'envelope')
   subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',*os.environ.get('STREAMOO_SPECTRUM_ENVELOPE_SANITIZE','').split(),'-Icomponents/spectrum/include','components/spectrum/streamoo_spectrum_envelope.c','tests/spectrum_envelope_test.c','-o',binary],cwd=ROOT,check=True)
   subprocess.run([binary],check=True)
