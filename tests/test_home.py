import ctypes as C
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_canvas import Canvas, Rect
from test_ui import UI, Text, Clock, HomeSnapshot, POST

ROOT = Path(__file__).resolve().parents[1]


class HomeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        library = Path(cls.temp.name) / 'home.so'
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC',
                        '-Icomponents/graphics/include', '-Icomponents/text/include',
                        '-Icomponents/ui_widgets/include', '-Icomponents/ui/include', '-Imain',
                        'components/graphics/streamoo_canvas.c', 'components/text/streamoo_text.c',
                        'components/text/streamoo_text_layout.c', 'components/text/streamoo_font5x8.c',
                        'components/text/streamoo_bitmap_font.c', 'components/text/streamoo_ui_font.c',
                        'components/ui_widgets/streamoo_visualization.c', 'components/ui/streamoo_ui.c',
                        'components/ui/streamoo_ui_home.c', 'components/ui/streamoo_ui_list.c',
                        'main/home_probe.c', '-o', str(library)], cwd=ROOT, check=True)
        cls.lib = C.CDLL(str(library))
        cls.lib.streamoo_ui_init.argtypes = [C.POINTER(UI), C.c_uint16, POST, C.c_void_p]
        cls.lib.streamoo_ui_update_home.argtypes = [C.POINTER(UI), C.POINTER(HomeSnapshot), C.c_uint64]
        cls.lib.streamoo_ui_update_home.restype = C.c_bool
        cls.lib.streamoo_ui_draw_home.argtypes = [C.POINTER(UI), C.POINTER(Canvas), C.c_uint64]
        cls.lib.streamoo_ui_draw_home.restype = C.c_bool
        cls.lib.streamoo_ui_set_playing.argtypes = [C.POINTER(UI), C.c_uint64]
        cls.lib.streamoo_ui_set_visualization.argtypes = [C.POINTER(UI), C.c_int]
        cls.lib.streamoo_ui_set_visualization.restype = C.c_bool
        cls.lib.streamoo_ui_open_stations.argtypes = [C.POINTER(UI), C.c_uint64]
        cls.lib.streamoo_canvas_init.argtypes = [C.POINTER(Canvas), C.POINTER(C.c_uint8), C.c_size_t,
                                                C.c_int16, C.c_int16, C.c_uint16]
        cls.lib.streamoo_canvas_clip.argtypes = [C.POINTER(Canvas), Rect]
        cls.lib.streamoo_home_probe_init.argtypes = [C.POINTER(UI)]
        cls.lib.streamoo_home_probe_draw.argtypes = [C.POINTER(UI), C.POINTER(C.c_uint8), C.c_size_t, C.c_uint64]

    def setUp(self):
        self.ui = UI()
        self.commands = []
        def post(context, command):
            self.commands.append((command.contents.station_id, command.contents.catalog_generation))
            return True
        self.callback = POST(post)
        self.lib.streamoo_ui_init(C.byref(self.ui), 160, self.callback, None)
        self.models = []  # Keep borrowed strings alive for the entire UI lifetime.
        self.surface()

    def surface(self):
        self.guard = (C.c_uint8 * 8194)(*([0xa5] * 8194))
        self.pointer = C.cast(C.byref(self.guard, 1), C.POINTER(C.c_uint8))
        self.canvas = Canvas()
        self.lib.streamoo_canvas_init(C.byref(self.canvas), self.pointer, 8192, 256, 64, 128)

    def snapshot(self):
        s = HomeSnapshot()
        s.station_id = s.data_station_id = 1
        s.stream_generation = s.data_generation = 10
        s.text_revision = 1
        s.station_name = Text('TEST: Łódź'.encode(), len('TEST: Łódź'.encode()))
        s.title = Text(b'Old title', 9)
        s.description = Text(b'Old description', 15)
        s.codec = Text(b'MP3', 3)
        s.bitrate_kbps = 192
        s.wifi_connected = s.rssi_valid = s.metadata_valid = s.audio_info_valid = True
        s.clock_valid = s.visualization_valid = True
        s.rssi_dbm = -58
        s.clock = Clock(2026, 10, 24, 12, 34, 56)
        s.visualization.band_count = 24
        for i in range(24):
            s.visualization.bands[i] = (i + 1) * 1000
        s.visualization.rms[0], s.visualization.rms[1] = 8192, 24576
        s.visualization.peak[0], s.visualization.peak[1] = 16384, 32767
        self.models.append(s)
        return s

    def apply(self, s, now=1000):
        return self.lib.streamoo_ui_update_home(C.byref(self.ui), C.byref(s), now)

    def draw(self, now=1000):
        self.assertTrue(self.lib.streamoo_ui_draw_home(C.byref(self.ui), C.byref(self.canvas), now))
        self.assertEqual((self.guard[0], self.guard[-1]), (0xa5, 0xa5))
        return bytes(self.guard[1:-1])

    def test_initial_unknown_state_has_no_fake_metadata(self):
        frame = self.draw()
        self.assertEqual(frame[16 * 128:32 * 128], bytes(16 * 128))
        self.assertFalse(self.ui.home.clock_valid)
        self.assertFalse(self.ui.home.visualization_valid)
        self.assertEqual(self.commands, [])

    def test_station_change_discards_stale_stream_data(self):
        first = self.snapshot()
        self.assertTrue(self.apply(first))
        self.draw()
        changed = self.snapshot()
        changed.station_id = 2
        changed.stream_generation = 11
        changed.station_name = Text(b'New station', 11)
        # Old tags plus even invalid old pointer lengths must never be read.
        changed.title = Text(None, 999)
        changed.codec = Text(None, 999)
        changed.visualization.band_count = 255
        self.assertTrue(self.apply(changed, 2000))
        self.assertEqual((self.ui.home.title.bytes, self.ui.home.description.bytes, self.ui.home.codec.bytes), (0, 0, 0))
        self.assertFalse(self.ui.home.visualization_valid)
        frame = self.draw(2000)
        self.assertEqual(frame[16 * 128:32 * 128], bytes(16 * 128))
        self.assertEqual(self.ui.playing_id, 2)
        self.assertEqual(self.commands, [])

    def test_old_snapshot_and_revision_cannot_restore_old_metadata(self):
        first = self.snapshot()
        self.assertTrue(self.apply(first))
        current = self.snapshot()
        current.station_id = current.data_station_id = 2
        current.stream_generation = current.data_generation = 11
        self.assertTrue(self.apply(current, 2000))
        before = bytes(self.ui)
        self.assertFalse(self.apply(first, 3000))
        self.assertEqual(bytes(self.ui), before)
        current.text_revision = 3
        self.assertTrue(self.apply(current, 4000))
        older = self.snapshot()
        older.station_id = older.data_station_id = 2
        older.stream_generation = older.data_generation = 11
        self.assertFalse(self.apply(older, 5000))
        self.assertEqual(self.ui.home.text_revision, 3)

    def test_lost_metadata_clears_rows_but_retains_current_codec(self):
        s = self.snapshot()
        self.assertTrue(self.apply(s))
        self.draw()
        s.metadata_valid = False
        s.title = Text(None, 999)
        self.assertTrue(self.apply(s, 2000))
        frame = self.draw(2000)
        self.assertEqual(frame[16 * 128:32 * 128], bytes(16 * 128))
        self.assertTrue(self.ui.home.audio_info_valid)
        self.assertEqual(self.ui.home.bitrate_kbps, 192)

    def test_visualization_bridges_short_gap_but_not_expired_or_new_stream(self):
        snapshot = self.snapshot()
        snapshot.visualization.bands[5] = 32767
        self.assertTrue(self.apply(snapshot, 1000))
        self.assertEqual(self.ui.home.visualization.bands[5], 32767)
        snapshot.visualization_valid = False
        snapshot.visualization = type(snapshot.visualization)()
        snapshot.text_revision = 2
        self.assertTrue(self.apply(snapshot, 1700))
        self.assertTrue(self.ui.home.visualization_valid)
        self.assertEqual(self.ui.home.visualization.bands[5], 4095)
        self.assertTrue(self.apply(snapshot, 1750))
        self.assertEqual(self.ui.home.visualization.bands[5], 2047)
        self.assertTrue(self.apply(snapshot, 1801))
        self.assertFalse(self.ui.home.visualization_valid)
        self.assertEqual(self.ui.home.visualization.bands[5], 0)
        snapshot.station_id = snapshot.data_station_id = 2
        snapshot.stream_generation = snapshot.data_generation = 11
        snapshot.text_revision = 2
        self.assertTrue(self.apply(snapshot, 1900))
        self.assertFalse(self.ui.home.visualization_valid)

    def test_clock_remains_visible_when_wifi_is_offline(self):
        s = self.snapshot()
        self.assertTrue(self.apply(s))
        before = self.draw()
        s.wifi_connected = False
        s.rssi_dbm = 1000  # inactive RSSI is intentionally irrelevant
        self.assertTrue(self.apply(s, 1100))
        after = self.draw(1100)
        for y in range(32, 64):
            self.assertEqual(before[y * 128 + 88:(y + 1) * 128], after[y * 128 + 88:(y + 1) * 128])
        self.assertNotEqual(before[52 * 128:64 * 128], after[52 * 128:64 * 128])

    def test_calendar_and_active_fields_validated_before_update(self):
        s = self.snapshot()
        self.assertTrue(self.apply(s))
        for clock in (Clock(2023, 2, 29, 12, 0, 0), Clock(1900, 2, 29, 0, 0, 0),
                      Clock(2026, 13, 1, 0, 0, 0), Clock(2026, 1, 1, 24, 0, 0)):
            s.clock = clock
            before = bytes(self.ui)
            self.assertFalse(self.apply(s, 2000))
            self.assertEqual(bytes(self.ui), before)
        s.clock = Clock(2000, 2, 29, 23, 59, 59)
        self.assertTrue(self.apply(s))
        s.rssi_dbm = 1
        self.assertFalse(self.apply(s))
        s.rssi_dbm = -127
        s.title = Text(None, 2)
        self.assertFalse(self.apply(s))

    def test_scroll_resets_only_when_identity_or_text_changes(self):
        s = self.snapshot()
        self.assertTrue(self.apply(s, 1000))
        s.clock.second = 57
        self.assertTrue(self.apply(s, 1200))
        self.assertEqual(self.ui.home_text_start_ms, 1000)
        s.text_revision = 2
        self.assertTrue(self.apply(s, 1500))
        self.assertEqual(self.ui.home_text_start_ms, 1500)
        s.station_id = s.data_station_id = 2
        s.stream_generation = s.data_generation = 11
        s.text_revision = 0
        self.assertTrue(self.apply(s, 2000))
        self.assertEqual(self.ui.home_text_start_ms, 2000)

    def test_playing_update_immediately_clears_old_metadata(self):
        s = self.snapshot()
        self.assertTrue(self.apply(s))
        self.lib.streamoo_ui_set_playing(C.byref(self.ui), 2)
        self.assertFalse(self.ui.home.metadata_valid)
        self.assertFalse(self.ui.home.audio_info_valid)
        self.assertFalse(self.ui.home.visualization_valid)
        frame = self.draw()
        self.assertEqual(frame[16 * 128:32 * 128], bytes(16 * 128))
        self.assertFalse(self.apply(s, 2000))

    def test_renderer_keeps_ui_immutable_and_respects_parent_clip(self):
        s = self.snapshot()
        self.assertTrue(self.apply(s))
        clip = Rect(20, 10, 200, 40)
        self.lib.streamoo_canvas_clip(C.byref(self.canvas), clip)
        before = bytes(self.ui)
        self.draw()
        self.assertEqual(bytes(self.ui), before)
        for y in range(64):
            for x in range(256):
                if not (20 <= x < 220 and 10 <= y < 50):
                    pixel = (self.guard[1 + y * 128 + x // 2] >> (0 if x & 1 else 4)) & 15
                    self.assertEqual(pixel, 5 if x & 1 else 10)

    def test_widget_selection_and_home_list_isolation(self):
        s = self.snapshot()
        self.assertTrue(self.apply(s))
        frames = []
        for kind in range(3):
            self.assertTrue(self.lib.streamoo_ui_set_visualization(C.byref(self.ui), kind))
            frames.append(self.draw())
        self.assertNotEqual(frames[0], frames[1])
        self.assertNotEqual(frames[1], frames[2])
        self.assertFalse(self.lib.streamoo_ui_set_visualization(C.byref(self.ui), 99))
        self.assertEqual(self.ui.visualization, 2)
        self.lib.streamoo_ui_open_stations(C.byref(self.ui), 2000)
        before = bytes(self.guard)
        self.assertFalse(self.lib.streamoo_ui_draw_home(C.byref(self.ui), C.byref(self.canvas), 2000))
        self.assertEqual(bytes(self.guard), before)

    def test_bitrate_and_rssi_extremes_stay_in_bounds(self):
        s = self.snapshot()
        s.bitrate_kbps = 0xffffffff
        s.rssi_dbm = -127
        s.clock = Clock(9999, 12, 31, 23, 59, 59)
        self.assertTrue(self.apply(s))
        self.draw()

    def test_diagnostic_stages_match_reviewed_frames(self):
        probe = UI()
        self.assertTrue(self.lib.streamoo_home_probe_init(C.byref(probe)))
        frame = (C.c_uint8 * 8192)()
        for name, time in (('connected', 0), ('waiting', 12500), ('updated', 15500), ('offline', 18500)):
            self.assertTrue(self.lib.streamoo_home_probe_draw(C.byref(probe), frame, 8192, time))
            self.assertEqual(bytes(frame), (ROOT / f'tests/fixtures/home_{name}.4bpp').read_bytes())
        self.assertFalse(probe.home.metadata_valid)
        self.assertFalse(probe.home.clock_valid)
