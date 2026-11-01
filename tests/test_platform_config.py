import importlib.util
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('check_config', ROOT / 'tools/check_config.py')
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


class PlatformTests(unittest.TestCase):
    def test_normal_radio_defaults_preserve_accepted_flac_crypto_and_tcp_profile(self):
        data=mod.parse(ROOT/'sdkconfig.defaults')
        for symbol in ('FLAC_DR','MBEDTLS_HARDWARE_AES','MBEDTLS_HARDWARE_SHA','CRYPTO_EARLY_INIT'):
            self.assertEqual(data[symbol],'y')
        self.assertEqual(data['LWIP_TCP_WND_DEFAULT'],'23040')
        self.assertEqual(data['LWIP_TCP_RECVMBOX_SIZE'],'18')
        self.assertEqual(data['LWIP_SNTP_MAX_SERVERS'],'3')
        self.assertNotIn('LWIP_SNTP_STARTUP_DELAY',data)
        self.assertIn('# CONFIG_LWIP_SNTP_STARTUP_DELAY is not set',(ROOT/'sdkconfig.defaults').read_text())

    def test_radio_defaults_disable_unused_fixture_buffers_but_keep_i2s_cache_safe(self):
        defaults=(ROOT/'sdkconfig.defaults').read_text()
        for symbol in ('STREAMOO_AUDIO_CONTROL_TEST','STREAMOO_I2S_SMOKE_TEST'):
            self.assertIn('# CONFIG_'+symbol+' is not set',defaults)
        self.assertEqual(mod.parse(ROOT/'sdkconfig.defaults')['I2S_ISR_IRAM_SAFE'],'y')

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

    def test_cpu_profile_uses_stable_64_bit_esp_timer(self):
        data = mod.parse(ROOT / 'sdkconfig.diagnostics.defaults')
        self.assertEqual(data['FREERTOS_GENERATE_RUN_TIME_STATS'], 'y')
        self.assertEqual(data['FREERTOS_RUN_TIME_COUNTER_TYPE_U64'], 'y')
        self.assertEqual(data['FREERTOS_RUN_TIME_STATS_USING_ESP_TIMER'], 'y')

    def test_s3_vector_fft_is_explicit_and_bounded(self):
        data = mod.parse(ROOT / 'sdkconfig.defaults')
        self.assertEqual(data['DSP_ENABLE_SIMD'], 'y')
        self.assertEqual(data['DSP_OPTIMIZED'], 'y')
        self.assertEqual(data['DSP_MAX_FFT_SIZE_2048'], 'y')
        manifest = (ROOT / 'components/spectrum/idf_component.yml').read_text()
        self.assertIn('espressif/esp-dsp:', manifest)
        self.assertIn('version: "==1.8.2"', manifest)
