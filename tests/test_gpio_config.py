"""Host tests; also discoverable by pytest, with no third-party dependencies."""
import ctypes
import importlib.util
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('check_config', ROOT / 'tools/check_config.py')
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


class Pin(ctypes.Structure):
    _fields_ = [('name', ctypes.c_char_p), ('gpio', ctypes.c_int), ('optional', ctypes.c_int)]


class GPIOTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        output = Path(cls.temp.name) / 'validator.so'
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC',
                        '-I', str(ROOT / 'components/board_config/include'),
                        str(ROOT / 'components/board_config/streamoo_gpio.c'), '-o', str(output)],
                       check=True)
        cls.library = ctypes.CDLL(str(output))
        cls.validator = cls.library.streamoo_gpio_validate
        cls.validator.argtypes = [ctypes.POINTER(Pin), ctypes.c_size_t, ctypes.c_int,
                                  ctypes.c_int, ctypes.POINTER(ctypes.c_size_t)]
        cls.validator.restype = ctypes.c_int

    def setUp(self):
        self.config = mod.parse(ROOT / 'sdkconfig.defaults')

    def validate_c(self, usb=1, uart=0):
        pins = (Pin * len(mod.PINS))(*[
            Pin(key.encode(), int(self.config[key]), key == 'LCD_RST') for key in mod.PINS])
        return self.validator(pins, len(pins), usb, uart, None)

    def test_default(self):
        self.assertEqual(mod.check(self.config), [])
        self.assertEqual(self.validate_c(), 0)

    def test_reset_conflict(self):
        self.config['LCD_RST'] = self.config['LCD_CS']
        self.assertTrue(any('conflict' in error for error in mod.check(self.config)))
        self.assertNotEqual(self.validate_c(), 0)

    def test_exhaustive_host_firmware_parity(self):
        # 11 signals x 53 candidates x 4 console policies = 2332 configurations.
        for usb in (0, 1):
            for uart in (0, 1):
                for name in mod.PINS:
                    for pin in range(-2, 51):
                        with self.subTest(usb=usb, uart=uart, name=name, pin=pin):
                            self.setUp()
                            self.config.update(RESERVE_USB='y' if usb else 'n',
                                               ESP_CONSOLE_UART_DEFAULT='y' if uart else 'n')
                            self.config[name] = str(pin)
                            self.assertEqual(self.validate_c(usb, uart) == 0,
                                             mod.check(self.config) == [])

    def test_custom_console_conflict(self):
        self.config.update(ESP_CONSOLE_UART_CUSTOM='y', ESP_CONSOLE_UART_TX_GPIO='11',
                           ESP_CONSOLE_UART_RX_GPIO='-1')
        self.assertTrue(any('conflict' in error for error in mod.check(self.config)))

    def test_invalid_input(self):
        for value in ('bad', None):
            self.config['LCD_MOSI'] = value
            self.assertTrue(mod.check(self.config))
        del self.config['LCD_RST']
        self.assertTrue(any('Missing LCD_RST' in error for error in mod.check(self.config)))


if __name__ == '__main__':
    unittest.main()
