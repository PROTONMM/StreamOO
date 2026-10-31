"""Original bounded M3U/PLS resolver, without network/SDK."""
import ctypes as C
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class PlaylistResolverTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup)
  target=Path(cls.tmp.name)/'resolver.so'
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC','-Icomponents/streaming/include','components/streaming/streamoo_playlist_resolver.c','-o',str(target)],cwd=ROOT,check=True)
  cls.lib=C.CDLL(str(target));cls.lib.streamoo_playlist_resolve.argtypes=[C.c_void_p,C.c_size_t,C.c_char_p,C.c_void_p];cls.lib.streamoo_playlist_resolve.restype=C.c_int
 def resolve(self,doc,base='https://radio.invalid/a/b/list.m3u'):
  if isinstance(doc,str):doc=doc.encode()
  buf=C.create_string_buffer(doc);out=C.create_string_buffer(2049)
  result=self.lib.streamoo_playlist_resolve(buf,len(doc),base.encode(),out)
  return result,out.value.decode()
 def test_m3u_bom_comments_and_first_entry(self):
  self.assertEqual(self.resolve('\ufeff#EXTM3U\r\n#EXTINF:-1,Zażółć\r\nhttps://stream.invalid/live\r\nhttps://other.invalid/live\n'),(0,'https://stream.invalid/live'))
 def test_pls_numerical_order_not_line_order(self):
  self.assertEqual(self.resolve('[playlist]\nFile2=https://b.invalid/live\nTitle1=Music\nFile1=https://a.invalid/live\nLength1=-1\nNumberOfEntries=2\nVersion=2'),(0,'https://a.invalid/live'))
 def test_relative_urls(self):
  for value,expected in [('live.mp3','https://radio.invalid/a/b/live.mp3'),('../live','https://radio.invalid/a/live'),('../../live','https://radio.invalid/live'),('../../../live','https://radio.invalid/live'),('/live','https://radio.invalid/live'),('//stream.invalid/live','https://stream.invalid/live'),('./live?x=../q','https://radio.invalid/a/b/live?x=../q'),('?x=1','https://radio.invalid/a/b/list.m3u?x=1'),('?url=https://a.invalid','https://radio.invalid/a/b/list.m3u?url=https://a.invalid'),('live?url=https://a.invalid','https://radio.invalid/a/b/live?url=https://a.invalid')]:
   with self.subTest(value=value):self.assertEqual(self.resolve(value),(0,expected))
  self.assertEqual(self.resolve('live','https://radio.invalid'),(0,'https://radio.invalid/live'))
 def test_hls_rejected_even_after_first_url(self):
  for doc in ['#EXTM3U\n#EXT-X-TARGETDURATION:6\nsegment.ts','#EXTM3U\nhttps://radio.invalid/audio\n#EXT-X-STREAM-INF:BANDWIDTH=128000\nother.m3u8']:
   self.assertEqual(self.resolve(doc)[0],2)
 def test_reject_unsafe_or_malformed(self):
  for doc in ['',b'abc\x00def','http://radio.invalid/live','ftp://radio.invalid/live','https://user:pass@radio.invalid/live','https://radio.invalid/#secret','file:///etc/passwd','[playlist]\nFile9999999999=https://radio.invalid','[playlist]\nFile0=https://radio.invalid','https://radio.invalid/\\bad','x'*16385,'#EXTM3U\n#comment']:
   with self.subTest(doc=str(doc)[:50]):self.assertEqual(self.resolve(doc)[0],1)
 def test_size_and_url_boundaries(self):
  self.assertEqual(self.resolve('#'+'x'*16383)[0],1)
  url='https://radio.invalid/'+'x'*(2048-len('https://radio.invalid/'))
  self.assertEqual(self.resolve(url),(0,url))
  self.assertEqual(self.resolve(url+'x')[0],1)
 def test_native_bounds_corpus(self):
  import os
  base=Path(self.tmp.name)
  source=base/'bounds.c';binary=base/'bounds'
  source.write_text(r'''
#include "streamoo_playlist_resolver.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
int main(void){
 const char *cases[]={"#EXTM3U\n../radio", "[playlist]\r\nFile2=https://b.invalid\r\nFile1=/audio\r\n", "#EXTM3U\nhttp://unsafe.invalid", "#EXT-X-MEDIA-SEQUENCE:1", "https://radio.invalid/a/../../../b?x=../", "[playlist]\nFile999999999999999999999=x"};
 char out[2049];
 for(unsigned i=0;i<sizeof cases/sizeof *cases;++i){size_t n=strlen(cases[i]);char *p=malloc(n+1);assert(p);memcpy(p,cases[i],n+1);streamoo_playlist_resolve(p,n,"https://base.invalid/x/list.m3u",out);free(p);}
 unsigned seed=1;
 for(unsigned i=0;i<3000;++i){size_t n=i%193;char *p=malloc(n+1);assert(p);for(size_t j=0;j<n;++j){seed=seed*1664525u+1013904223u;p[j]=(char)(seed>>24);}p[n]=0;streamoo_playlist_resolve(p,n,"https://base.invalid/a/b",out);free(p);}
 char *p=malloc(STREAMOO_PLAYLIST_LIMIT+2);assert(p);memset(p,'x',STREAMOO_PLAYLIST_LIMIT+2);
 assert(streamoo_playlist_resolve(p,STREAMOO_PLAYLIST_LIMIT+1,"https://base.invalid",out)==STREAMOO_PLAYLIST_INVALID);
 p[0]='#';assert(streamoo_playlist_resolve(p,STREAMOO_PLAYLIST_LIMIT,"https://base.invalid",out)==STREAMOO_PLAYLIST_INVALID);free(p);
 return 0;
}
''')
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',*os.environ.get('STREAMOO_PLAYLIST_SANITIZE','').split(),'-Icomponents/streaming/include',str(source),'components/streaming/streamoo_playlist_resolver.c','-o',str(binary)],cwd=ROOT,check=True)
  subprocess.run([str(binary)],check=True)
