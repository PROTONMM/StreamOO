"""Original CRC frame boundaries checked against FFprobe packets, without vendor fakes."""
import ctypes as C
import json,os,random,shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class Frames(C.Structure):
 _fields_=[('data',C.c_void_p),('bytes',C.c_size_t),('scan',C.c_size_t),('ready',C.c_size_t),('crc',C.c_uint16)]
class FLACFramesTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup);base=Path(cls.tmp.name)
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC','-Icomponents/decoder','components/decoder/streamoo_flac_frames.c','components/decoder/streamoo_flac_header.c','-o',str(base/'frames.so')],cwd=ROOT,check=True)
  cls.lib=C.CDLL(str(base/'frames.so'));cls.lib.streamoo_flac_frames_init.argtypes=[C.POINTER(Frames),C.c_void_p];cls.lib.streamoo_flac_frames_append.argtypes=[C.POINTER(Frames),C.c_void_p,C.c_size_t];cls.lib.streamoo_flac_frames_append.restype=C.c_size_t;cls.lib.streamoo_flac_frames_find.argtypes=[C.POINTER(Frames),C.c_bool,C.c_size_t];cls.lib.streamoo_flac_frames_find.restype=C.c_int;cls.lib.streamoo_flac_frames_consume.argtypes=[C.POINTER(Frames)]
 def split(self,data,chunk,corrupt=False):
  storage=C.create_string_buffer(131088);s=Frames();self.lib.streamoo_flac_frames_init(C.byref(s),storage);frames=[]
  for at in range(0,len(data),chunk):
   part=data[at:at+chunk];buf=C.create_string_buffer(part);copied=self.lib.streamoo_flac_frames_append(C.byref(s),buf,len(part))
   if corrupt and copied!=len(part):return None
   self.assertEqual(copied,len(part))
   while True:
    r=self.lib.streamoo_flac_frames_find(C.byref(s),at+len(part)==len(data),131072)
    if r<0:
     if corrupt:return None
     self.fail('valid frame rejected')
    if not r:break
    frames.append(storage.raw[:s.ready]);self.lib.streamoo_flac_frames_consume(C.byref(s))
    if not s.bytes:break
  self.assertEqual(s.bytes,0);return frames
 def test_real_frames_at_arbitrary_chunks_and_midstream_join(self):
  if not shutil.which('ffmpeg'):self.skipTest('FFmpeg required for independent oracle')
  base=Path(self.tmp.name)
  for name,data in [('noise',random.Random(71).randbytes(4608*6*12)),('silence',bytes(4608*6*12))]:
   raw=base/(name+'.s24le');raw.write_bytes(data);encoded=base/(name+'.flac')
   subprocess.run(['ffmpeg','-v','error','-f','s24le','-ar','44100','-ac','2','-i',str(raw),'-c:a','flac','-y',str(encoded)],check=True)
   packets=json.loads(subprocess.check_output(['ffprobe','-v','error','-show_packets','-of','json',str(encoded)]))['packets'];content=encoded.read_bytes();expected=[content[int(p['pos']):int(p['pos'])+int(p['size'])] for p in packets]
   for chunk in (1,7,257,4096):
    with self.subTest(signal=name,chunk=chunk):self.assertEqual(self.split(b''.join(expected),chunk),expected)
   self.assertEqual(self.split(b''.join(expected[3:]),257),expected[3:])
   # Later frames may reference STREAMINFO for rate/depth; their header
   # CRC8 and frame CRC16 must still validate without changing frame bodies.
   from test_flac_header import crc8
   def crc16(data):
    value=0
    for b in data:
     value^=b<<8
     for _ in range(8):value=((value<<1)^(0x8005 if value&32768 else 0))&65535
    return value
   converted=[expected[0]]
   self.lib.streamoo_flac_boundary_header.argtypes=[C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
   for original in expected[1:]:
    buf=C.create_string_buffer(original[:16]);length=C.c_size_t();self.assertEqual(self.lib.streamoo_flac_boundary_header(buf,min(16,len(original)),C.byref(length)),2)
    changed=bytearray(original);changed[2]&=0xf0;changed[3]&=0xf1
    changed[length.value-1]=crc8(changed[:length.value-1]);changed[-2:]=crc16(changed[:-2]).to_bytes(2,'big');converted.append(bytes(changed))
   self.assertEqual(self.split(b''.join(converted),257),converted)
   repaired=base/(name+'-references.flac');repaired.write_bytes(content[:int(packets[0]['pos'])]+b''.join(converted))
   def pcm(path):return subprocess.check_output(['ffmpeg','-v','error','-i',str(path),'-f','s32le','-'])
   self.assertEqual(pcm(encoded),pcm(repaired))
   broken=bytearray(b''.join(expected));broken[len(expected[0])+len(expected[1])-1]^=1
   self.assertIsNone(self.split(bytes(broken),257,True))
 def test_native_bounds_and_false_sync_with_sanitizers(self):
  base=Path(self.tmp.name);source=base/'bounds.c';binary=base/'bounds'
  source.write_text(r'''#include "streamoo_flac_frames.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
int main(void){uint8_t *p=malloc(STREAMOO_FLAC_FRAME_STORAGE);streamoo_flac_frames_t s;assert(p);
 streamoo_flac_frames_init(&s,p);uint8_t garbage[257];memset(garbage,0xff,sizeof garbage);
 int r=0;while(!r){size_t n=streamoo_flac_frames_append(&s,garbage,sizeof garbage);assert(n);r=streamoo_flac_frames_find(&s,false,131072);}assert(r==-1);
 streamoo_flac_frames_init(&s,p);assert(streamoo_flac_frames_find(&s,true,131072)==-1);free(p);return 0;}''')
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',*os.environ.get('STREAMOO_FLAC_FRAMES_SANITIZE','').split(),'-Icomponents/decoder',str(source),'components/decoder/streamoo_flac_frames.c','components/decoder/streamoo_flac_header.c','-o',str(binary)],cwd=ROOT,check=True);subprocess.run([str(binary)],check=True)
