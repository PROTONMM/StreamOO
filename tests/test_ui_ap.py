from pathlib import Path
import ctypes as C
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class APDisplayTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.temp.cleanup)
        lib=Path(cls.temp.name)/'ap.so'
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',
            '-Icomponents/ui/include','-Icomponents/graphics/include','-Icomponents/text/include',
            'components/ui/streamoo_ui_ap.c','components/graphics/streamoo_canvas.c',
            'components/text/streamoo_text.c','components/text/streamoo_font5x8.c','-o',str(lib)],cwd=ROOT,check=True)
        cls.lib=C.CDLL(str(lib));cls.draw=cls.lib.streamoo_ui_ap_draw
        cls.draw.argtypes=[C.c_void_p,C.c_size_t,C.c_void_p,C.c_size_t,C.c_void_p,C.c_size_t];cls.draw.restype=C.c_bool
        cls.glyph=cls.lib.streamoo_font5x8;cls.glyph.argtypes=[C.c_void_p,C.c_uint32,C.c_void_p];cls.glyph.restype=C.c_bool
    def expected_row(self,text,y):
        row=bytearray(8*128)
        for i,code in enumerate(text):
            cols=(C.c_uint8*5)();self.assertTrue(self.glyph(None,code,cols))
            for x,col in enumerate(cols):
                for dy in range(8):
                    if col&(1<<dy):
                        px=i*6+x;off=dy*128+px//2;row[off]|=15<<(0 if px%2 else 4)
        return bytes(row)
    def test_max_password_has_all_characters_in_two_rows(self):
        password=bytes(range(33,96));frame=C.create_string_buffer(8192)
        self.assertTrue(self.draw(frame,8192,b'StreamOO-123ABC',15,password,len(password)))
        self.assertEqual(frame.raw[32*128:40*128],self.expected_row(password[:42],32))
        self.assertEqual(frame.raw[40*128:48*128],self.expected_row(password[42:],40))
    def test_shorter_password_erases_previous_second_row(self):
        frame=C.create_string_buffer(8192)
        self.assertTrue(self.draw(frame,8192,b'StreamOO-123ABC',15,b'x'*63,63))
        self.assertTrue(self.draw(frame,8192,b'StreamOO-123ABC',15,b'x'*8,8))
        self.assertEqual(frame.raw[40*128:48*128],bytes(8*128))
    def test_bounds_and_frame_canaries(self):
        frame=(C.c_uint8*8194)(*([77]*8194));pointer=C.cast(C.byref(frame,1),C.c_void_p)
        self.assertFalse(self.draw(pointer,8191,b'StreamOO-123ABC',15,b'x'*8,8))
        self.assertEqual(bytes(frame),bytes([77])*8194)
        self.assertTrue(self.draw(pointer,8192,b'StreamOO-123ABC',15,b'x'*8,8))
        self.assertEqual(frame[0],77);self.assertEqual(frame[8193],77)
    def test_bad_credentials_rejected_before_draw(self):
        for password in (b'',b'x'*7,b'x'*64,b'x'*8+b'\0',b'x'*8+b'\xff'):
            frame=(C.c_uint8*8192)(*([77]*8192))
            self.assertFalse(self.draw(frame,8192,b'StreamOO-123ABC',15,password,len(password)))
            self.assertEqual(bytes(frame),bytes([77])*8192)
    def test_sta_screen_replaces_ap_credentials_and_waits_without_ip(self):
        draw=self.lib.streamoo_ui_sta_draw;draw.argtypes=self.draw.argtypes;draw.restype=C.c_bool
        frame=C.create_string_buffer(8192)
        self.assertTrue(self.draw(frame,8192,b'StreamOO-123ABC',15,b'x'*63,63))
        self.assertTrue(draw(frame,8192,b'Router',6,b'192.168.1.73',12))
        self.assertEqual(frame.raw[40*128:48*128],bytes(8*128))
        self.assertTrue(draw(frame,8192,None,0,None,0))
        before=frame.raw;self.assertFalse(draw(frame,8192,b'Router',6,b'bad-ip',6));self.assertEqual(before,frame.raw)
