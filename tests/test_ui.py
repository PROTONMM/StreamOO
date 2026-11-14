import ctypes as C
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_canvas import Canvas, Rect
from test_visualization import Model as VisualizationModel

ROOT = Path(__file__).resolve().parents[1]


class Station(C.Structure):
    _fields_ = [('stable_id', C.c_uint64), ('name', C.c_char_p), ('name_bytes', C.c_size_t)]


class Command(C.Structure):
    _fields_ = [('station_id', C.c_uint64), ('catalog_generation', C.c_uint64)]


POST = C.CFUNCTYPE(C.c_bool, C.c_void_p, C.POINTER(Command))


class Text(C.Structure):
    _fields_ = [('data', C.c_char_p), ('bytes', C.c_size_t)]


class Clock(C.Structure):
    _fields_ = [('year', C.c_uint16)] + [(name, C.c_uint8) for name in
                                        ('month', 'day', 'hour', 'minute', 'second')]


class HomeSnapshot(C.Structure):
    _fields_ = [(name, C.c_uint64) for name in
                ('station_id', 'stream_generation', 'data_station_id', 'data_generation', 'text_revision')] + [
        (name, Text) for name in ('station_name', 'title', 'description', 'codec')] + [
        ('bitrate_kbps', C.c_uint32), ('rssi_dbm', C.c_int16)] + [
        (name, C.c_bool) for name in ('wifi_connected', 'rssi_valid', 'metadata_valid',
                                     'audio_info_valid', 'clock_valid', 'visualization_valid', 'bitrate_measured')] + [
        ('clock', Clock), ('visualization', VisualizationModel)]


class UI(C.Structure):
    _fields_ = [('screen', C.c_int), ('stations', C.POINTER(Station)),
                ('count', C.c_uint32), ('selected', C.c_uint32),
                ('catalog_generation', C.c_uint64), ('playing_id', C.c_uint64),
                ('animation_from_q16', C.c_int64), ('animation_start_ms', C.c_uint64),
                ('selection_start_ms', C.c_uint64), ('animation_ms', C.c_uint16),
                ('post', POST), ('post_context', C.c_void_p), ('home', HomeSnapshot),
                ('home_text_start_ms', C.c_uint64), ('visualization_valid_at_ms', C.c_uint64),
                ('visualization_update_at_ms', C.c_uint64),
                ('visualization_valid_kind', C.c_int), ('home_received', C.c_bool),
                ('home_scroll_reset_pending', C.c_bool), ('visualization', C.c_int),('running',C.c_bool),('last_activity_ms',C.c_uint64)]


class Probe(C.Structure):
    _fields_ = [('ui', UI), ('next_event', C.c_uint), ('requested_id', C.c_uint64)]


class UITests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        library = Path(cls.temp.name) / 'ui.so'
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC',
                        '-Icomponents/graphics/include', '-Icomponents/text/include', '-Icomponents/ui/include', '-Icomponents/ui_widgets/include', '-Imain',
                        'components/graphics/streamoo_canvas.c', 'components/text/streamoo_text.c',
                        'components/text/streamoo_text_layout.c', 'components/text/streamoo_font5x8.c',
                        'components/text/streamoo_bitmap_font.c', 'components/text/streamoo_ui_font.c',
                        'components/ui/streamoo_ui.c', 'components/ui/streamoo_ui_list.c',
                        'components/ui/streamoo_ui_home.c', 'components/ui_widgets/streamoo_visualization.c', 'main/ui_list_probe.c',
                        '-o', str(library)], cwd=ROOT, check=True)
        cls.lib = C.CDLL(str(library))
        signatures = {
            'init': ([C.POINTER(UI), C.c_uint16, POST, C.c_void_p], C.c_bool),
            'set_catalog': ([C.POINTER(UI), C.POINTER(Station), C.c_uint32, C.c_uint64], C.c_bool),
            'set_playing': ([C.POINTER(UI), C.c_uint64], None),
            'open_stations': ([C.POINTER(UI), C.c_uint64], C.c_bool),
            'move': ([C.POINTER(UI), C.c_int32, C.c_uint64], C.c_bool),
            'cancel': ([C.POINTER(UI)], C.c_bool),
            'confirm': ([C.POINTER(UI)], C.c_bool),
            'position_q16': ([C.POINTER(UI), C.c_uint64], C.c_int64),
            'draw_list': ([C.POINTER(UI), C.POINTER(Canvas), Rect, C.c_uint64], C.c_bool),
        }
        for name, (args, result) in signatures.items():
            function = getattr(cls.lib, 'streamoo_ui_' + name)
            function.argtypes, function.restype = args, result
        cls.lib.streamoo_canvas_init.argtypes = [C.POINTER(Canvas), C.POINTER(C.c_uint8), C.c_size_t,
                                                C.c_int16, C.c_int16, C.c_uint16]
        cls.lib.streamoo_canvas_clip.argtypes = [C.POINTER(Canvas), Rect]
        cls.lib.streamoo_ui_list_probe_init.argtypes = [C.POINTER(Probe)]
        cls.lib.streamoo_ui_list_probe_init.restype = C.c_bool
        cls.lib.streamoo_ui_list_probe_draw.argtypes = [C.POINTER(Probe), C.POINTER(C.c_uint8),
                                                       C.c_size_t, C.c_uint64]
        cls.lib.streamoo_ui_list_probe_draw.restype = C.c_bool

    def setUp(self):
        self.ui = UI()
        self.accept = True
        self.commands = []

        def post(context, pointer):
            if self.accept:
                self.commands.append((pointer.contents.station_id, pointer.contents.catalog_generation))
            return self.accept
        self.callback = POST(post)
        self.names = [text.encode() for text in ('TEST: Łódź', 'TEST: Gdańsk', 'TEST: Poznań',
                                                'TEST: Żory', 'TEST: Piąta')]
        self.ids = (90, 12, 77, 4, 1002)
        self.catalog = (Station * 5)(*[Station(code, name, len(name)) for code, name in zip(self.ids, self.names)])
        self.assertTrue(self.lib.streamoo_ui_init(C.byref(self.ui), 160, self.callback, None))
        self.assertTrue(self.lib.streamoo_ui_set_catalog(C.byref(self.ui), self.catalog, 5, 123))
        self.lib.streamoo_ui_set_playing(C.byref(self.ui), 77)

    def open(self, now=1000):
        self.assertTrue(self.lib.streamoo_ui_open_stations(C.byref(self.ui), now))

    def position(self, now):
        return self.lib.streamoo_ui_position_q16(C.byref(self.ui), now)

    def test_browsing_and_cancel_never_issue_a_player_command(self):
        self.open()
        self.assertEqual(self.ui.selected, 2)
        self.assertTrue(self.lib.streamoo_ui_move(C.byref(self.ui), 1, 1100))
        self.assertEqual(self.ui.playing_id, 77)
        self.lib.streamoo_ui_set_playing(C.byref(self.ui), 90)
        self.assertEqual(self.ui.selected, 3)  # incoming snapshot does not hijack cursor
        self.assertTrue(self.lib.streamoo_ui_cancel(C.byref(self.ui)))
        self.assertEqual((self.ui.screen, self.ui.playing_id, self.commands), (0, 90, []))

    def test_confirm_uses_stable_id_and_generation_not_index(self):
        self.open()
        self.lib.streamoo_ui_move(C.byref(self.ui), 1, 1100)
        self.assertTrue(self.lib.streamoo_ui_confirm(C.byref(self.ui)))
        self.assertEqual(self.commands, [(4, 123)])
        self.assertEqual((self.ui.screen, self.ui.playing_id), (0, 77))
        self.open(1200)
        self.assertEqual(self.ui.selected, 2)  # player has not acknowledged command
        self.lib.streamoo_ui_cancel(C.byref(self.ui))
        self.lib.streamoo_ui_set_playing(C.byref(self.ui), 4)
        self.open(1300)
        self.assertEqual(self.ui.selected, 3)

    def test_queue_full_retains_selection_and_allows_retry(self):
        self.open()
        self.lib.streamoo_ui_move(C.byref(self.ui), 2, 1100)
        self.accept = False
        self.assertFalse(self.lib.streamoo_ui_confirm(C.byref(self.ui)))
        self.assertEqual((self.ui.screen, self.ui.selected, self.commands), (1, 4, []))
        self.accept = True
        self.assertTrue(self.lib.streamoo_ui_confirm(C.byref(self.ui)))
        self.assertEqual(self.commands, [(1002, 123)])

    def test_animation_retarget_and_rapid_confirm_preserve_latest_target(self):
        self.open()
        self.lib.streamoo_ui_move(C.byref(self.ui), 1, 1000)
        self.assertEqual(self.position(1000), 2 * 65536)
        self.assertEqual(self.position(1080), 2 * 65536 + 32768)
        self.lib.streamoo_ui_move(C.byref(self.ui), 1, 1080)
        self.assertEqual(self.position(1080), 2 * 65536 + 32768)
        self.assertEqual(self.position(1240), 4 * 65536)
        self.assertTrue(self.lib.streamoo_ui_confirm(C.byref(self.ui)))
        self.assertEqual(self.commands, [(1002, 123)])

    def test_large_deltas_clamp_without_losing_valid_target(self):
        self.open()
        self.lib.streamoo_ui_move(C.byref(self.ui), 2147483647, 1000)
        self.assertEqual(self.ui.selected, 4)
        self.lib.streamoo_ui_move(C.byref(self.ui), -2147483648, 1080)
        self.assertEqual(self.ui.selected, 0)
        self.assertGreaterEqual(self.position(1100), 0)
        self.assertEqual(self.position(1240), 0)

    def test_snapshot_replacement_closes_list_and_reopens_by_id(self):
        self.lib.streamoo_ui_set_playing(C.byref(self.ui), 90)
        self.open()
        self.lib.streamoo_ui_move(C.byref(self.ui), 4, 1100)
        self.reversed = (Station * 5)(*reversed(self.catalog))
        self.assertTrue(self.lib.streamoo_ui_set_catalog(C.byref(self.ui), self.reversed, 5, 124))
        self.assertEqual((self.ui.screen, self.ui.playing_id, self.commands), (0, 90, []))
        self.open(1200)
        self.assertEqual(self.ui.selected, 4)
        self.assertTrue(self.lib.streamoo_ui_confirm(C.byref(self.ui)))
        self.assertEqual(self.commands, []) # confirming the playing station is a no-op

    def test_empty_and_invalid_catalog(self):
        invalid = (Station * 1)(Station(0, b'bad', 3))
        self.assertFalse(self.lib.streamoo_ui_set_catalog(C.byref(self.ui), invalid, 1, 2))
        self.assertEqual((self.ui.count, self.ui.catalog_generation), (5, 123))
        self.assertTrue(self.lib.streamoo_ui_set_catalog(C.byref(self.ui), None, 0, 2))
        self.open()
        self.assertFalse(self.lib.streamoo_ui_confirm(C.byref(self.ui)))
        self.assertFalse(self.lib.streamoo_ui_move(C.byref(self.ui), 1, 1200))
        self.assertTrue(self.lib.streamoo_ui_cancel(C.byref(self.ui)))
        self.assertEqual(self.commands, [])

    def surface(self):
        self.guard = (C.c_uint8 * 8194)(*([0xa5] * 8194))
        self.pointer = C.cast(C.byref(self.guard, 1), C.POINTER(C.c_uint8))
        self.canvas = Canvas()
        self.lib.streamoo_canvas_init(C.byref(self.canvas), self.pointer, 8192, 256, 64, 128)

    def test_renderer_respects_parent_clip_and_does_not_mutate_ui(self):
        self.open()
        self.surface()
        clip = Rect(20, 10, 200, 40)
        self.lib.streamoo_canvas_clip(C.byref(self.canvas), clip)
        before = bytes(self.ui)
        self.assertTrue(self.lib.streamoo_ui_draw_list(C.byref(self.ui), C.byref(self.canvas),
                                                      Rect(0, 8, 256, 56), 1000))
        self.assertEqual(bytes(self.ui), before)
        for y in range(64):
            for x in range(256):
                if not (20 <= x < 220 and 10 <= y < 50):
                    value = (self.guard[1 + y * 128 + x // 2] >> (0 if x & 1 else 4)) & 15
                    self.assertEqual(value, 5 if x & 1 else 10)
        self.assertEqual((self.guard[0], self.guard[-1]), (0xa5, 0xa5))

    def test_selection_is_brightest_and_neighbours_are_dimmed(self):
        self.open()
        self.surface()
        self.assertTrue(self.lib.streamoo_ui_draw_list(C.byref(self.ui), C.byref(self.canvas),
                                                      Rect(0, 8, 256, 56), 1000))
        def max_gray(top, bottom):
            values = self.guard[1 + top * 128:1 + bottom * 128]
            return max(max(value >> 4, value & 15) for value in values)
        self.assertEqual(max_gray(28, 44), 15)
        self.assertLessEqual(max_gray(12, 28), 8)
        self.assertLessEqual(max_gray(44, 60), 8)

    def test_instant_animation_and_unknown_playing_id(self):
        self.ui.animation_ms = 0
        self.lib.streamoo_ui_set_playing(C.byref(self.ui), 9999)
        self.open()
        self.assertEqual(self.ui.selected, 0)
        self.lib.streamoo_ui_move(C.byref(self.ui), 2, 1001)
        self.assertEqual(self.position(1001), 2 * 65536)

    def test_diagnostic_script_selects_without_changing_playing_snapshot(self):
        probe = Probe()
        self.assertTrue(self.lib.streamoo_ui_list_probe_init(C.byref(probe)))
        frame = (C.c_uint8 * 8192)()
        self.assertTrue(self.lib.streamoo_ui_list_probe_draw(C.byref(probe), frame, 8192, 0))
        self.assertEqual(bytes(frame), (ROOT / 'tests/fixtures/station_list.4bpp').read_bytes())
        for time in (1080, 2160, 3160, 4160, 5500):
            self.assertTrue(self.lib.streamoo_ui_list_probe_draw(C.byref(probe), frame, 8192, time))
        self.assertEqual((probe.ui.screen, probe.requested_id, probe.ui.playing_id), (0, 1003, 1002))
        self.assertTrue(self.lib.streamoo_ui_list_probe_draw(C.byref(probe), frame, 8192, 6600))
        self.assertEqual((probe.ui.screen, probe.ui.selected), (1, 1))
