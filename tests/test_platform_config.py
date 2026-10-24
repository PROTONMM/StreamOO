import importlib.util
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('check_config', ROOT / 'tools/check_config.py')
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


class PlatformTests(unittest.TestCase):
    def test_versioned_defaults_match_n16r8(self):
        self.assertEqual(mod.check_platform(mod.parse(ROOT / 'sdkconfig.defaults')), [])

    def test_wrong_platform_options_rejected(self):
        data = mod.parse(ROOT / 'sdkconfig.defaults')
        keys = ('IDF_TARGET', 'ESPTOOLPY_FLASHSIZE_16MB', 'SPIRAM', 'SPIRAM_MODE_OCT',
                'SPIRAM_SPEED_80M', 'SPIRAM_USE_MALLOC', 'SPIRAM_MEMTEST',
                'ESP_DEFAULT_CPU_FREQ_MHZ_240')
        for key in keys:
            with self.subTest(key=key):
                broken = dict(data)
                broken[key] = 'n'
                self.assertEqual(len(mod.check_platform(broken)), 1)

    def test_missing_options_rejected(self):
        self.assertEqual(len(mod.check_platform({})), 8)
