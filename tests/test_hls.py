"""Bounded RFC8216 parser: real C, no network or vendor decoder claims."""
import ctypes as C
import os
from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class Segment(C.Structure):
 _fields_=[('url',C.c_char*2049),('discontinuity',C.c_bool)]
class Manifest(C.Structure):
 _fields_=[('sequence',C.c_uint64),('target_s',C.c_uint32),('count',C.c_uint32),('endlist',C.c_bool),('variant',C.c_char*2049),('segments',Segment*32),('bitrate_bps',C.c_uint32)]
class HLSTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup);base=Path(cls.tmp.name)
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC','-Icomponents/streaming/include','components/streaming/streamoo_hls.c','components/streaming/streamoo_playlist_resolver.c','-o',str(base/'hls.so')],cwd=ROOT,check=True)
  cls.lib=C.CDLL(str(base/'hls.so'));cls.lib.streamoo_hls_parse.argtypes=[C.c_void_p,C.c_size_t,C.c_char_p,C.POINTER(Manifest)]
  cls.lib.streamoo_hls_detect.argtypes=[C.c_void_p,C.c_size_t];cls.lib.streamoo_hls_detect.restype=C.c_bool
 def parse(self,text,base=b'https://radio.invalid/hls/index.m3u8'):
  data=text.encode() if isinstance(text,str) else text;m=Manifest();buf=C.create_string_buffer(data)
  return self.lib.streamoo_hls_parse(buf,len(data),base,C.byref(m)),m
 def test_media_sequence_crlf_query_and_discontinuity(self):
  result,m=self.parse('#EXTM3U\r\n#EXT-X-TARGETDURATION:6\r\n#EXT-X-MEDIA-SEQUENCE:99\r\n#EXTINF:5.5,\r\na.aac?token=x\r\n#EXT-X-DISCONTINUITY\r\n#EXTINF:6,\r\n../b.ts\r\n#EXT-X-ENDLIST\r\n')
  self.assertEqual((result,m.sequence,m.target_s,m.count,m.endlist),(0,99,6,2,True));self.assertEqual(m.segments[0].url,b'https://radio.invalid/hls/a.aac?token=x');self.assertEqual(m.segments[1].url,b'https://radio.invalid/b.ts');self.assertTrue(m.segments[1].discontinuity)
 def test_master_selects_audio_lowest_bitrate(self):
  result,m=self.parse('#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=1000000,RESOLUTION=1280x720,CODECS="avc1.42,mp4a.40.2"\nvideo.m3u8\n#EXT-X-STREAM-INF:BANDWIDTH=128000,CODECS="mp4a.40.2"\nhi.m3u8\n#EXT-X-STREAM-INF:BANDWIDTH=64000,CODECS="mp4a.40.5"\nlow.m3u8\n')
  self.assertEqual(result,1);self.assertEqual(m.variant,b'https://radio.invalid/hls/low.m3u8');self.assertEqual(m.bitrate_bps,64000)
 def test_master_alternate_audio_overrides_video(self):
  result,m=self.parse('#EXTM3U\n#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID="audio",NAME="PL",DEFAULT=YES,URI="polish.m3u8"\n#EXT-X-STREAM-INF:BANDWIDTH=128000,CODECS="mp4a.40.2"\nother.m3u8\n')
  self.assertEqual(result,1);self.assertTrue(m.variant.endswith(b'/polish.m3u8'))
 def test_unsupported_features_and_downgrade(self):
  for tag in ('#EXT-X-KEY:METHOD=AES-128,URI="key"','#EXT-X-MAP:URI="init.mp4"','#EXT-X-BYTERANGE:10@0','#EXT-X-PART:DURATION=0.3,URI="part"','#EXT-X-I-FRAMES-ONLY','#EXT-X-GAP'):
   with self.subTest(tag=tag):self.assertEqual(self.parse('#EXTM3U\n'+tag+'\n')[0],3)
  self.assertEqual(self.parse('#EXTM3U\n#EXT-X-TARGETDURATION:6\n#EXTINF:6,\nhttp://radio.invalid/a.aac\n')[0],2)
 def test_malformed_bounds_embedded_nul_and_overflow(self):
  for text in ('','#EXTM3U\n#EXTINF:6,\na.aac\n','#EXTM3U\n#EXT-X-TARGETDURATION:0\n','#EXTM3U\n#EXT-X-TARGETDURATION:6\na.aac\n','#EXTM3U\n#EXT-X-TARGETDURATION:6\n#EXT-X-MEDIA-SEQUENCE:18446744073709551616\n','#EXTM3U\0\n','#EXTM3U\n#EXT-X-TARGETDURATION:6\n'+('#EXTINF:6,\na.aac\n'*33),'x'*16385):
   with self.subTest(text=text[:64]):self.assertEqual(self.parse(text)[0],2)
 def test_detection_does_not_mutate_or_confuse_m3u(self):
  for text,expected in ((b'#EXTM3U\nhttp://x.invalid/a',False),(b'#EXTM3U\n#EXT-X-VERSION:3\n',True),(b'#EXTINF:1,Not #EXT-X-HLS\n',False)):
   buf=C.create_string_buffer(text);self.assertEqual(self.lib.streamoo_hls_detect(buf,len(text)),expected);self.assertEqual(buf.value,text)
 def test_native_bounds_with_sanitizers(self):
  # Executable, not a sanitized shared library loaded into unsanitized Python.
  base=Path(self.tmp.name);src=base/'fuzz.c';binary=base/'bounds'
  src.write_text(r'''#include "streamoo_hls.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
int main(void){char *p=malloc(16386);streamoo_hls_manifest_t *m=malloc(sizeof *m);assert(p&&m);
 const char *valid="#EXTM3U\n#EXT-X-TARGETDURATION:6\n#EXTINF:6,\na.aac\n";
 for(size_t n=0;n<strlen(valid);++n){memcpy(p,valid,n);streamoo_hls_parse(p,n,"https://x.invalid/a",m);}
 unsigned seed=7;for(unsigned i=0;i<20000;++i){size_t n=i%16386;for(size_t j=0;j<n;++j){seed=seed*1664525+1013904223;p[j]=(char)(seed>>24);}streamoo_hls_detect((unsigned char *)p,n);streamoo_hls_parse(p,n,"https://x.invalid/a",m);}
 free(m);free(p);return 0;}''')
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',*os.environ.get('STREAMOO_HLS_SANITIZE','').split(),'-Icomponents/streaming/include',str(src),'components/streaming/streamoo_hls.c','components/streaming/streamoo_playlist_resolver.c','-o',str(binary)],cwd=ROOT,check=True)
  subprocess.run([str(binary)],check=True,timeout=20)

 def test_selected_variant_average_and_invalid_average_fallback(self):
  for average,expected in [('96000',96000),('0',128000),('-1',128000),('129000',128000),('99999999999999999999999999',128000),('64.5',128000)]:
   with self.subTest(average=average):
    result,m=self.parse('#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=128000,AVERAGE-BANDWIDTH='+average+'\naudio.m3u8\n')
    self.assertEqual((result,m.bitrate_bps),(1,expected))
 def test_audio_rendition_has_no_unrelated_variant_bitrate(self):
  audio='#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID="a",NAME="PL",DEFAULT=YES,URI="pl.m3u8"\n'
  variant='#EXT-X-STREAM-INF:BANDWIDTH=128000\nother.m3u8\n'
  for body in (audio+variant,variant+audio):
   with self.subTest(body=body):
    result,m=self.parse('#EXTM3U\n'+body);self.assertEqual((result,m.bitrate_bps),(1,0));self.assertTrue(m.variant.endswith(b'/pl.m3u8'))
