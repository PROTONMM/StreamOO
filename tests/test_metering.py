"""Actual consumer lifecycle with a deterministic pthread-backed task scheduler."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest
from test_network_player import HEADERS
ROOT = Path(__file__).resolve().parents[1]
class MeteringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        base = Path(cls.temp.name)
        (base / 'freertos').mkdir()
        headers = dict(HEADERS)
        headers['freertos/FreeRTOS.h'] = '''#pragma once
#include <pthread.h>
typedef pthread_mutex_t portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED PTHREAD_MUTEX_INITIALIZER
#define portENTER_CRITICAL(p) pthread_mutex_lock(p)
#define portEXIT_CRITICAL(p) pthread_mutex_unlock(p)
#define pdPASS 1
#define pdMS_TO_TICKS(x) (x)
'''
        headers['freertos/task.h'] += 'void vTaskDelete(void *);\nint xTaskCreatePinnedToCore(void (*)(void *),const char *,unsigned,void *,unsigned,void *,int);\n'
        for name, body in headers.items(): (base / name).write_text(body)
        cls.binary = str(base / 'metering')
        includes = ['-I' + str(base)] + ['-Icomponents/' + n + '/include'
            for n in ('metering', 'spectrum', 'levels', 'pcm', 'audio_gain')]
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-pthread',
            *os.environ.get('STREAMOO_METERING_SANITIZE', '').split(), *includes,
            'components/metering/streamoo_metering.c', 'components/spectrum/streamoo_spectrum.c', 'components/spectrum/streamoo_spectrum_stream.c', 'components/spectrum/streamoo_spectrum_envelope.c', 'components/levels/streamoo_ballistics.c', 'components/levels/streamoo_levels.c',
            'components/pcm/streamoo_pcm.c', 'components/audio_gain/streamoo_audio_gain.c',
            'tests/metering_test.c', '-lm', '-o', cls.binary], cwd=ROOT, check=True)
    def case(self, n): subprocess.run([self.binary, str(n)], check=True, timeout=10)
    def test_allocation_and_task_failure_cleanup(self): self.case(0)
    def test_worker_levels_generation_and_ack_before_free(self): self.case(1)
    def test_stale_measurements_expire(self): self.case(2)
    def test_slow_consumer_drops_and_resets_window(self): self.case(3)
    def test_new_session_has_no_old_levels(self): self.case(4)
    def test_backward_clock_invalidates_result(self): self.case(5)

    def test_display_cadence_uses_smoothed_levels(self): self.case(6)

    def test_spectrum_selected_only_and_reenable_resets(self): self.case(7)
    def test_psram_failure_keeps_rms_working(self): self.case(8)
