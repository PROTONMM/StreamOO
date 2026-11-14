from pathlib import Path
import os
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class PreferencesTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        cls.binary = str(Path(cls.temp.name) / 'preferences')
        flags = os.environ.get('STREAMOO_PREFERENCES_SANITIZE', '').split()
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', *flags,
                        '-Icomponents/preferences/include',
                        'components/preferences/streamoo_preferences_policy.c',
                        'tests/preferences_test.c', '-o', cls.binary], cwd=ROOT, check=True)

    def run_case(self, scenario):
        subprocess.run([self.binary, str(scenario)], check=True)

    def test_all_values_roundtrip(self): self.run_case(0)
    def test_invalid_blob_preserves_output(self): self.run_case(1)
    def test_idle_boot_and_delayed_ack(self): self.run_case(2)
    def test_changes_delay_and_revert_cancels(self): self.run_case(3)
    def test_coalesced_changes_still_delay(self): self.run_case(4)
    def test_failure_retry_without_ack(self): self.run_case(5)
    def test_long_uptime(self): self.run_case(6)
    def test_invalid_policy_input(self): self.run_case(7)
