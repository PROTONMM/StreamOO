from pathlib import Path
import os
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['components/graphics/streamoo_canvas.c'] + [
    'components/text/' + name + '.c' for name in
    ('streamoo_text', 'streamoo_text_layout', 'streamoo_font5x8',
     'streamoo_bitmap_font', 'streamoo_ui_font')] + [
    'components/ui/' + name + '.c' for name in
    ('streamoo_ui_connection_row', 'streamoo_ui', 'streamoo_ui_list', 'streamoo_ui_home', 'streamoo_ui_controls')] + [
    'components/ui_widgets/streamoo_visualization.c', 'components/ui_widgets/streamoo_volume_bar.c', 'main/encoder_demo.c', 'tests/controls_test.c']

class ControlsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        cls.binary = str(Path(cls.temp.name) / 'controls')
        flags = os.environ.get('STREAMOO_CONTROLS_SANITIZE', '').split()
        includes = ['-Icomponents/' + name + '/include' for name in
                    ('graphics', 'text', 'ui', 'ui_widgets', 'input')]
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', *flags,
                        *includes, '-Imain', *SOURCES, '-o', cls.binary], cwd=ROOT, check=True)

    def run_case(self, scenario):
        subprocess.run([self.binary, str(scenario)], check=True)

    def test_mode_a(self): self.run_case(0)
    def test_mode_b(self): self.run_case(1)
    def test_visualization_wrap_and_no_volume(self): self.run_case(2)
    def test_list_confirm_cancel_and_isolation(self): self.run_case(3)
    def test_rejected_commands_preserve_state(self): self.run_case(4)
    def test_invalid_and_empty(self): self.run_case(5)
    def test_demo_1000_frames_and_station_selection(self): self.run_case(6)

    def test_external_audio_owner_and_rejection(self): self.run_case(7)

    def test_home_ip_volume_bar_in_third_row_and_clear(self): self.run_case(8)

    def test_live_catalog_selection_reorder_and_removal(self): self.run_case(9)

    def test_network_selection_waits_for_owner_ack(self): self.run_case(10)

    def test_live_levels_and_unavailable_spectrum(self): self.run_case(11)

    def test_live_codec_rssi_copied_validated_and_cleared(self): self.run_case(12)

    def test_live_clock_independent_of_station_and_marquee(self): self.run_case(13)
