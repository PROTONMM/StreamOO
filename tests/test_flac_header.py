"""Original RFC9639 adapter; generated FLAC also validated by independent FFmpeg."""
import ctypes as C
import os
from pathlib import Path
import shutil,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
def crc8(data):
 value=int.from_bytes(data,'big')<<8
 for i in range(len(data)*8-1,-1,-1):
  if value & (1<<(i+8)):value^=0x107<<i
 return value

def frame(block=12,rate=9,bits=4,channel=1,extra=b'',variable=False,number=b'\0'):
 data=bytes([255,249 if variable else 248,(block<<4)|rate,(channel<<4)|(bits<<1)])+number+extra
 return data+bytes([crc8(data)])
class FLACHeaderTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup);base=Path(cls.tmp.name)
  lib=base/'header.so'
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC','-Icomponents/decoder','components/decoder/streamoo_flac_header.c','-o',str(lib)],cwd=ROOT,check=True)
  cls.lib=C.CDLL(str(lib));cls.lib.streamoo_flac_header.argtypes=[C.c_void_p,C.c_size_t,C.c_size_t,C.c_void_p,C.POINTER(C.c_size_t)];cls.lib.streamoo_flac_header.restype=C.c_int
 def parse(self,data,capacity=65536):
  source=C.create_string_buffer(data);out=C.create_string_buffer(42);n=C.c_size_t()
  result=self.lib.streamoo_flac_header(source,len(data),capacity,out,C.byref(n))
  return result,out.raw,n.value
 def test_observed_live_header(self):
  code,info,n=self.parse(bytes.fromhex('fff859acf7be9d9b00'))
  self.assertEqual((code,n),(2,9));self.assertEqual(info[:8],b'fLaC\x80\0\0\x22')
  self.assertEqual(int.from_bytes(info[8:10],'big'),4608)
  self.assertEqual(int.from_bytes(info[10:12],'big'),4608)
  word=int.from_bytes(info[18:26],'big')
  self.assertEqual((word>>44,((word>>41)&7)+1,((word>>36)&31)+1,word&((1<<36)-1)),(44100,2,24,0))
  self.assertEqual(info[26:],b'\0'*16)
 def test_partial_and_file_prefix(self):
  data=frame()
  for n in range(len(data)):self.assertEqual(self.parse(data[:n])[0],0)
  self.assertEqual(self.parse(b'fLaC')[0],1)
 def test_depth_rate_block_channel_matrix(self):
  for bits,width in [(4,16),(6,24),(7,32)]:
   for channel,channels in [(0,1),(1,2),(8,2),(9,2),(10,2)]:
    for rate,expected,extra in [(4,8000,b''),(9,44100,b''),(10,48000,b''),(12,44000,b'\x2c'),(13,44100,b'\xac\x44'),(14,44100,b'\x11\x3a')]:
     with self.subTest(bits=bits,channel=channel,rate=rate):
      status,info,_=self.parse(frame(block=8,rate=rate,bits=bits,channel=channel,extra=extra))
      self.assertEqual(status,2);packed=int.from_bytes(info[18:26],'big')
      self.assertEqual((packed>>44,((packed>>41)&7)+1,((packed>>36)&31)+1),(expected,channels,width))
  for code,extra,expected in [(1,b'',192),(2,b'',576),(5,b'',4608),(6,b'\xff',256),(7,b'\x0f\xff',4096),(8,b'',256),(13,b'',8192)]:
   self.assertEqual(int.from_bytes(self.parse(frame(block=code,extra=extra))[1][10:12],'big'),expected)
 def test_reject_crc_reserved_missing_or_out_of_range(self):
  valid=frame()
  for i in range(len(valid)):
   broken=bytearray(valid);broken[i]^=1
   self.assertEqual(self.parse(bytes(broken))[0],3)
  for data in [frame(rate=0),frame(bits=0),frame(bits=3),frame(bits=5),frame(channel=11),frame(channel=2),frame(rate=1),frame(block=0),frame(block=7,extra=b'\xff\xff'),frame(number=b'\xc0\x80'),frame(number=b'\x80'),frame(number=b'\xff'),frame(number=b'\xfe\x82\x80\x80\x80\x80\x80')]:
   with self.subTest(data=data.hex()):self.assertEqual(self.parse(data)[0],3)
  self.assertEqual(self.parse(valid,100)[0],3)
 def test_variable_coded_number_and_bound(self):
  status,info,n=self.parse(frame(block=8,variable=True,number=b'\xfe\x82\x80\x80\x80\x80\x80'))
  self.assertEqual((status,n),(2,12));self.assertEqual(int.from_bytes(info[8:10],'big'),16)
  self.assertEqual(int.from_bytes(info[10:12],'big'),16384)
 def test_native_bounds_corpus(self):
  base=Path(self.tmp.name);source=base/'bounds.c';binary=base/'bounds'
  source.write_text(r'''
#include "streamoo_flac_header.h"
#include <assert.h>
int main(void){uint8_t p[17],out[42];size_t used;unsigned seed=1;
for(unsigned i=0;i<10000;++i){size_t n=i%18;for(unsigned j=0;j<n;++j){seed=seed*1664525u+1013904223u;p[j]=(uint8_t)(seed>>24);}streamoo_flac_header(p,n,65536,out,&used);}
const uint8_t valid[]={255,248,89,172,247,190,157,155,0};assert(streamoo_flac_header(valid,sizeof valid,65536,out,&used)==STREAMOO_FLAC_FRAME);return 0;}
''')
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',*os.environ.get('STREAMOO_FLAC_SANITIZE','').split(),'-Icomponents/decoder',str(source),'components/decoder/streamoo_flac_header.c','-o',str(binary)],cwd=ROOT,check=True)
  subprocess.run([str(binary)],check=True)
 def test_header_reconstruction_preserves_real_flac_pcm(self):
  if not shutil.which('ffmpeg'):self.skipTest('FFmpeg needed for independent FLAC oracle')
  base=Path(self.tmp.name);original=base/'original.flac'
  subprocess.run(['ffmpeg','-v','error','-f','lavfi','-i','sine=frequency=997:sample_rate=44100:duration=0.4','-c:a','flac','-y',str(original)],check=True)
  data=original.read_bytes();at=4
  while True:
   last=data[at]&128;size=int.from_bytes(data[at+1:at+4],'big');at+=4+size
   if last:break
  frames=data[at:];status,info,n=self.parse(frames[:16]);self.assertEqual(status,2)
  repaired=base/'repaired.flac';repaired.write_bytes(info+frames)
  def decode(path):return subprocess.check_output(['ffmpeg','-v','error','-i',str(path),'-f','s32le','-'])
  self.assertEqual(decode(original),decode(repaired))

  packets=__import__('json').loads(subprocess.check_output(['ffprobe','-v','error','-show_packets','-of','json',str(original)]))['packets']
  second=packets[1];frames=data[int(second['pos']):]
  status,info,_=self.parse(frames[:16]);self.assertEqual(status,2)
  repaired.write_bytes(info+frames)
  # Joining a live stream after frame0 must retain only the remaining samples.
  skip=int(packets[0]['duration'])*4
  self.assertEqual(decode(original)[skip:],decode(repaired))
