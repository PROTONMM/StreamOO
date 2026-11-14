from pathlib import Path
import os,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class BallisticsTests(unittest.TestCase):
 def test_cadence_attack_release_peak_hold_and_reset(self):
  with tempfile.TemporaryDirectory() as tmp:
   binary=str(Path(tmp)/'ballistics')
   subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',*os.environ.get('STREAMOO_BALLISTICS_SANITIZE','').split(),'-Icomponents/levels/include','components/levels/streamoo_ballistics.c','tests/ballistics_test.c','-o',binary],cwd=ROOT,check=True)
   subprocess.run([binary],check=True)
