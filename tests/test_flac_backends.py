"""Real alternative libraries + real adapter, compared byte-for-byte with FFmpeg.
Only unused Xtensa vendor entry points are stubbed; no simulated FLAC decoder.
"""
import importlib.util
import json
import os
from pathlib import Path
import random
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
HARNESS=r'''
#include "streamoo_decoder.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv){
 assert(argc==6);FILE *f=fopen(argv[1],"rb"),*out=fopen(argv[2],"wb");assert(f&&out);
 fseek(f,0,SEEK_END);size_t n=(size_t)ftell(f);rewind(f);
 uint8_t *data=malloc(n),*pcm=malloc(1048576),*input=malloc(65536);assert(data&&pcm&&input);assert(fread(data,1,n,f)==n);fclose(f);
 size_t offset=(size_t)atoi(argv[4]),chunk=(size_t)atoi(argv[3]);int fault=atoi(argv[5]);
 streamoo_decoder_t *d=NULL;assert(streamoo_decoder_open(STREAMOO_CODEC_FLAC,&d)==ESP_OK);
 for(unsigned cycle=0;cycle<2;++cycle){
  size_t at=offset;unsigned calls=0,seed=7919;
  while(at<n){
   size_t limit=chunk;if(!limit){seed=seed*1664525u+1013904223u;limit=1+(seed%4096);}
   size_t take=n-at;if(take>limit)take=limit;
   assert(take<=65536);memcpy(input,data+at,take);
   streamoo_decode_result_t r={0};esp_err_t e=streamoo_decoder_process(d,input,take,at+take==n,pcm,1048576,&r);
   memset(input,0xa5,take); /* The network pump reuses its input buffer. */
   if(e!=ESP_OK){fprintf(stderr,"at=%zu chunk=%zu error=%d status=%d consumed=%zu produced=%zu needed=%zu\n",at,take,e,(int)r.codec_status,r.consumed,r.produced,r.needed);assert(fault);goto failed;}
   assert(r.consumed<=take&&r.produced<=1048576&&(r.consumed||r.produced));
   at+=r.consumed;if(r.produced){assert(r.format.bits==32&&r.format.channels<=2);assert(fwrite(pcm,1,r.produced,out)==r.produced);}
   assert(++calls<n*4+100);
  }
  assert(!fault);assert(streamoo_decoder_reset(d)==ESP_OK);
 }
failed:
 streamoo_decoder_close(d);free(input);free(pcm);free(data);fclose(out);return 0;
}
'''

class FlacBackendTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  if not shutil.which('ffmpeg'):raise unittest.SkipTest('FFmpeg oracle required')
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup);base=Path(cls.tmp.name)
  spec=importlib.util.spec_from_file_location('extended',ROOT/'tests/test_extended_decoder.py');mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
  (base/'esp_err.h').write_text('typedef int esp_err_t;\n#define ESP_OK 0\n#define ESP_FAIL -1\n#define ESP_ERR_INVALID_ARG 1\n#define ESP_ERR_NO_MEM 2\n#define ESP_ERR_INVALID_SIZE 3\n#define ESP_ERR_INVALID_RESPONSE 4\n')
  (base/'esp_log.h').write_text('#define ESP_LOGE(...) ((void)0)\n#define ESP_LOGI(...) ((void)0)\n')
  (base/'stubs.c').write_text(mod.HARNESS.split('int main(')[0])
  (base/'test.c').write_text(HARNESS)
  vendor=ROOT/'managed_components/espressif__esp_audio_codec/include'
  cls.includes=['-I'+str(base),'-Icomponents/decoder','-Icomponents/decoder/include','-Icomponents/pcm_adapter/include']+['-I'+str(vendor/p) for p in ['.','decoder','decoder/impl','simple_dec']]
  cls.binaries={}
  for name in ('micro','micro32','dr'):
   objects=[];flags=os.environ.get('STREAMOO_FLAC_BACKEND_SANITIZE','').split()
   variant=[]
   if name=='micro32':
    # Exercise the actual32-bit bit-reader branches on a64-bit host. This
    # changes only temporary source copies, never the pinned upstream source.
    override=base/'micro32_source'
    shutil.copytree(ROOT/'components/decoder/third_party/micro-flac',override)
    for relative in ('include/micro_flac/flac_decoder.h','src/compiler.h'):
     path=override/relative;header=path.read_text()
     assert header.count('#if UINTPTR_MAX == 0xFFFFFFFF')==1
     path.write_text(header.replace('#if UINTPTR_MAX == 0xFFFFFFFF','#if 1 /* StreamOO host test: force32-bit bit buffer only. */'))
    variant=['-I'+str(override/'include')]
   backend='dr' if name=='dr' else 'micro'
   common=['-O2','-g',*flags,*variant,*cls.includes,'-DCONFIG_STREAMOO_FLAC_'+backend.upper()+'=1']
   sources=['components/decoder/streamoo_decoder.c','components/decoder/streamoo_flac_header.c','components/decoder/streamoo_flac_parse.c',str(base/'stubs.c'),str(base/'test.c')]
   if name=='dr':sources+=['components/decoder/streamoo_flac_dr.c']
   else:
    sources+=['components/decoder/streamoo_flac_micro.cpp']
    source_root=str(override/'src') if name=='micro32' else 'components/decoder/third_party/micro-flac/src'
    sources+=[source_root+'/'+n+'.cpp' for n in ['crc','frame_header','pcm_packing','flac_decoder','decorrelation','lpc']]
   for i,source in enumerate(sources):
    obj=base/(name+str(i)+'.o');cpp=source.endswith('.cpp')
    extra=['-DMICRO_FLAC_DISABLE_OGG','-Icomponents/decoder/third_party/micro-flac/include'] if backend=='micro' else []
    subprocess.run(['c++' if cpp else 'cc','-std=c++14' if cpp else '-std=c11',*common,*extra,'-c',source,'-o',str(obj)],cwd=ROOT,check=True)
    objects.append(str(obj))
   binary=base/name;subprocess.run(['c++',*flags,*objects,'-o',str(binary)],check=True);cls.binaries[name]=binary

 def fixture(self,kind):
  base=Path(self.tmp.name);path=base/(kind+'.flac')
  if path.exists():return path
  if kind.startswith('noise'):
   raw=base/'noise.s24le';raw.write_bytes(random.Random(789).randbytes(4608*2*3*4+(7 if kind=='noise7' else 288)*2*3))
   args=['-f','s24le','-ar','44100','-ac','2','-i',str(raw)]
  elif kind=='silence':args=['-f','lavfi','-i','anullsrc=r=44100:cl=stereo','-t','0.4']
  else:args=['-f','lavfi','-i','sine=r=44100:f=997','-t','0.4']
  subprocess.run(['ffmpeg','-v','error',*args,'-c:a','flac','-y',str(path)],check=True)
  return path

 def compare(self,kind,raw=False):
  path=self.fixture(kind);base=Path(self.tmp.name);offset=0
  if raw:
   packet=json.loads(subprocess.check_output(['ffprobe','-v','error','-show_packets','-of','json',str(path)]))['packets'][1]
   offset=int(packet['pos'])
   # Join after the first audio frame; compare only the remaining samples.
   skip=int(json.loads(subprocess.check_output(['ffprobe','-v','error','-show_packets','-of','json',str(path)]))['packets'][0]['duration'])
  else:skip=0
  expected=subprocess.check_output(['ffmpeg','-v','error','-i',str(path),'-af',f'atrim=start_sample={skip}','-f','s32le','-'])
  for name,binary in self.binaries.items():
   for chunk in (0,1,7,257,4096,65536):
    with self.subTest(backend=name,kind=kind,chunk=chunk,raw=raw):
     output=base/'result.pcm';subprocess.run([str(binary),str(path),str(output),str(chunk),str(offset),'0'],check=True)
     self.assertEqual(output.read_bytes(),expected*2)

 def test_16bit_tone(self):self.compare('tone')
 def test_24bit_noise_and_short_final_block(self):self.compare('noise24')
 def test_tiny_silent_frames(self):self.compare('silence')
 def test_last_frame_with_seven_samples(self):self.compare('noise7')
 def test_live_join_after_first_frame(self):self.compare('noise24',raw=True)
 def test_corrupt_crc_fails(self):
  path=self.fixture('noise24');data=bytearray(path.read_bytes());data[-1]^=1
  broken=Path(self.tmp.name)/'broken.flac';broken.write_bytes(data)
  for name,binary in self.binaries.items():
   with self.subTest(backend=name):subprocess.run([str(binary),str(broken),str(Path(self.tmp.name)/'bad.pcm'),'4096','0','1'],check=True)
