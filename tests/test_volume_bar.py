"""Clipped 4bpp volume control and the third HOME row, no hardware dependencies."""
import ctypes as C
from pathlib import Path
import subprocess,tempfile,unittest
from test_canvas import Canvas,Rect
ROOT=Path(__file__).resolve().parents[1]
class VolumeBarTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup);lib=Path(cls.tmp.name)/'volume.so'
  sources=['components/graphics/streamoo_canvas.c','components/ui_widgets/streamoo_volume_bar.c','components/ui/streamoo_ui_connection_row.c']+['components/text/'+n+'.c' for n in ('streamoo_text','streamoo_text_layout','streamoo_font5x8')]
  includes=['-Icomponents/'+n+'/include' for n in ('graphics','ui_widgets','ui','text')]
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',*includes,*sources,'-o',str(lib)],cwd=ROOT,check=True)
  cls.lib=C.CDLL(str(lib));cls.init=cls.lib.streamoo_canvas_init;cls.init.argtypes=[C.POINTER(Canvas),C.POINTER(C.c_uint8),C.c_size_t,C.c_int16,C.c_int16,C.c_uint16];cls.init.restype=C.c_bool
  cls.bar=cls.lib.streamoo_volume_bar_draw;cls.bar.argtypes=[C.POINTER(Canvas),Rect,C.c_int,C.c_uint8];cls.bar.restype=C.c_bool
  cls.row=cls.lib.streamoo_ui_connection_row_draw;cls.row.argtypes=[C.POINTER(Canvas),C.c_char_p,C.c_size_t,C.c_int];cls.row.restype=C.c_bool
  cls.clip=cls.lib.streamoo_canvas_clip;cls.clip.argtypes=[C.POINTER(Canvas),Rect]
 def canvas(self):
  frame=(C.c_uint8*8194)();frame[0]=77;frame[-1]=77;c=Canvas();assert self.init(C.byref(c),C.cast(C.byref(frame,1),C.POINTER(C.c_uint8)),8192,256,64,128);return c,frame
 def pixel(self,frame,x,y):return (frame[1+y*128+x//2]>>(0 if x%2 else 4))&15
 def test_empty_half_full_and_monotonic(self):
  c,f=self.canvas();previous=-1
  for volume in range(101):
   self.assertTrue(self.bar(C.byref(c),Rect(176,25,80,6),volume,15))
   filled=sum(self.pixel(f,x,y)==15 for y in range(26,30) for x in range(177,255))
   self.assertGreaterEqual(filled,previous);previous=filled
   if volume in (0,50,100):self.assertEqual(filled,{0:0,50:156,100:312}[volume])
  self.assertEqual((f[0],f[-1]),(77,77))
 def test_inherited_clip_and_canaries(self):
  c,f=self.canvas();original=bytes(f);self.clip(C.byref(c),Rect(180,27,12,2))
  self.assertTrue(self.bar(C.byref(c),Rect(176,25,80,6),100,15))
  for y in range(64):
   for x in range(256):self.assertEqual(self.pixel(f,x,y),15 if 180<=x<192 and 27<=y<29 else 0)
  self.assertEqual((f[0],f[-1]),(original[0],original[-1]))
 def test_bad_input_does_not_write(self):
  c,f=self.canvas();before=bytes(f)
  for volume,gray,bounds in ((-1,15,Rect(0,0,80,6)),(101,15,Rect(0,0,80,6)),(50,16,Rect(0,0,80,6)),(50,15,Rect(0,0,2,6)),(50,15,Rect(32767,0,80,6))):
   self.assertFalse(self.bar(C.byref(c),bounds,volume,gray));self.assertEqual(bytes(f),before)
 def test_max_ip_dim_left_and_bright_volume(self):
  c,f=self.canvas();self.assertTrue(self.row(C.byref(c),b'255.255.255.255',15,100))
  left={self.pixel(f,x,y) for y in range(24,32) for x in range(128)}
  right={self.pixel(f,x,y) for y in range(24,32) for x in range(128,256)}
  self.assertEqual(left,{0,6});self.assertEqual(right,{0,15})
  self.assertTrue(any(self.pixel(f,x,y) for y in range(24,32) for x in range(4)))
  self.assertFalse(any(self.pixel(f,x,y) for y in list(range(24))+list(range(32,64)) for x in range(256)))
 def test_shorter_label_erases_previous_digits_and_fill(self):
  c,f=self.canvas();self.assertTrue(self.row(C.byref(c),b'10.0.0.42',9,100));self.assertTrue(self.row(C.byref(c),b'10.0.0.42',9,0))
  self.assertFalse(any(self.pixel(f,x,y) for y in range(24,32) for x in range(158,172)))
  self.assertFalse(any(self.pixel(f,x,y) for y in range(26,30) for x in range(177,255)))
 def test_row_rejects_bad_ip_and_volume(self):
  c,f=self.canvas();before=bytes(f)
  for ip,n,volume in ((b'',0,50),(b'x',1,50),(b'1'*16,16,50),(b'10.0.0.42',9,101)):
   self.assertFalse(self.row(C.byref(c),ip,n,volume));self.assertEqual(bytes(f),before)
