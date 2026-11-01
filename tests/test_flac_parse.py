"""Exercise the real parser callback against independent FFmpeg packet boundaries.

The SDK cache/frame decoder is binary Xtensa code: integration still needs S3.
"""
import ctypes as C
import json
import os
from pathlib import Path
import random
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class FlacParseTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.tmp.cleanup)
        base = Path(cls.tmp.name)
        # Keep vendor types/ABI inside C, not guessed ctypes struct layouts.
        (base / 'shim.c').write_text(r'''
#include "streamoo_flac_parse.h"
#include <string.h>
static esp_es_parse_frame_info_t info;
void reset(void){free(info.extra_data);memset(&info,0,sizeof info);}
int parse(uint8_t *p,unsigned n,int bos,int eos,unsigned *out){
 esp_es_parse_raw_t raw={.buffer=p,.len=n,.bos=bos,.eos=eos};
 int r=streamoo_flac_parse(&raw,&info);
 out[0]=info.frame_size;out[1]=info.skipped_size;out[2]=info.max_frame_size;
 out[3]=info.aud_info.sample_rate;out[4]=info.aud_info.channel;out[5]=info.aud_info.bits_per_sample;
 return r;
}
''')
        cls.binary = base / 'parser.so'
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                        '-shared', '-fPIC', '-Icomponents/decoder',
                        '-Imanaged_components/espressif__esp_audio_codec/include/simple_dec',
                        'components/decoder/streamoo_flac_parse.c',
                        'components/decoder/streamoo_flac_header.c', str(base / 'shim.c'),
                        '-o', str(cls.binary)], cwd=ROOT, check=True)
        cls.lib = C.CDLL(str(cls.binary))
        cls.lib.parse.argtypes = [C.c_void_p, C.c_uint, C.c_int, C.c_int, C.POINTER(C.c_uint)]
        cls.lib.parse.restype = C.c_int

    def setUp(self):
        self.lib.reset()

    def tearDown(self):
        self.lib.reset()

    def parse(self, data, bos=False, eos=False):
        out = (C.c_uint * 6)()
        result = self.lib.parse(C.create_string_buffer(bytes(data)), len(data), bos, eos, out)
        return result, list(out)

    def metadata(self):
        # 44.1 kHz, stereo, 24-bit, fixed 4608 sample blocks.
        return (b'fLaC\x80\0\0\x22\x12\0\x12\0' + b'\0' * 6
                + ((44100 << 44) | (1 << 41) | (23 << 36)).to_bytes(8, 'big') + b'\0' * 16)

    def packets(self, mode):
        if not shutil.which('ffmpeg') or not shutil.which('ffprobe'):
            self.skipTest('FFmpeg/FFprobe needed as independent FLAC oracle')
        base = Path(self.tmp.name)
        path = base / (mode + '.flac')
        if mode == 'noise':
            raw = base / 'noise.s24le'
            raw.write_bytes(random.Random(779).randbytes(4608 * 2 * 3 * 8))
            args = ['-f', 's24le', '-ar', '44100', '-ac', '2', '-i', str(raw)]
        else:
            signal = 'anullsrc=r=44100:cl=stereo' if mode == 'silence' else 'sine=r=44100:f=997'
            args = ['-f', 'lavfi', '-i', signal, '-t', '0.9']
        subprocess.run(['ffmpeg', '-v', 'error', *args, '-c:a', 'flac', '-y', str(path)], check=True)
        packets = json.loads(subprocess.check_output(['ffprobe', '-v', 'error',
                             '-show_packets', '-of', 'json', str(path)]))['packets']
        data = path.read_bytes()
        frames = [data[int(p['pos']):int(p['pos']) + int(p['size'])] for p in packets]
        return data, frames

    def test_bootstrap_and_supported_format(self):
        data = self.metadata()
        for n in range(42):
            self.assertEqual(self.parse(data[:n], bos=True)[0], -4)
        code, out = self.parse(data, bos=True)
        self.assertEqual((code, out[0], out[1], out[3:]), (0, 0, 42, [44100, 2, 24]))
        self.assertEqual(self.parse(b'xxxx' + data[4:], bos=True)[0], -9)
        invalid = bytearray(data)
        invalid[20] |= 14  # Eight channels.
        self.assertEqual(self.parse(invalid, bos=True)[0], -6)

    def test_each_frame_matches_ffprobe_including_tiny_silence(self):
        for mode in ('silence', 'tone', 'noise'):
            data, expected = self.packets(mode)
            for chunk in (1, 7, 257, 4096, 65536):
                with self.subTest(mode=mode, chunk=chunk):
                    self.lib.reset()
                    cache = b''
                    bos = True
                    got = []
                    at = 0
                    while at < len(data) or cache:
                        if at < len(data):
                            cache += data[at:at + chunk]
                            at += min(chunk, len(data) - at)
                        while cache:
                            code, out = self.parse(cache, bos=bos, eos=at == len(data))
                            if code == -4 or (code == 0 and out[0] == 0x7fffffff):
                                break
                            self.assertEqual(code, 0)
                            if out[1]:
                                cache = cache[out[1]:]
                                bos = False
                            else:
                                self.assertGreater(out[0], 0)
                                got.append(cache[:out[0]])
                                cache = cache[out[0]:]
                        if at == len(data) and cache:
                            self.fail('Parser stalled at EOF')
                    self.assertEqual(got, expected)
                    if mode == 'silence':
                        self.assertLess(max(map(len, got)), 64)
                    if mode == 'noise':
                        self.assertGreater(max(map(len, got)), 20480)

    def test_reparse_before_max_window_and_never_join_crc_valid_frames(self):
        _, frames = self.packets('silence')
        self.parse(self.metadata(), bos=True)
        first = frames[0]
        code, out = self.parse(first)
        self.assertEqual((code, out[0]), (0, 0x7fffffff))
        code, out = self.parse(first + frames[1][:16])
        self.assertEqual((code, out[0]), (0, len(first)))
        # A concatenation of CRC16-valid frames is itself CRC16-valid: still
        # return the first packet even at EOF, not a multi-second PCM burst.
        code, out = self.parse(b''.join(frames), eos=True)
        self.assertEqual((code, out[0]), (0, len(first)))

    def test_corruption_limit_and_truncated_eof(self):
        _, frames = self.packets('tone')
        self.parse(self.metadata(), bos=True)
        bad = bytearray(frames[0]); bad[-1] ^= 1
        self.assertEqual(self.parse(bad, eos=True)[0], -5)
        self.lib.reset(); self.parse(self.metadata(), bos=True)
        self.assertEqual(self.parse(frames[0][:-1], eos=True)[0], -5)
        self.lib.reset(); self.parse(self.metadata(), bos=True)
        self.assertEqual(self.parse(frames[0][:16] + b'\0' * (128 * 1024 + 16))[0], -6)

    def test_large_file_metadata_skipped_in_chunks(self):
        meta = bytearray(self.metadata()); meta[4] = 0
        self.parse(meta, bos=True)
        count = 200000
        code, out = self.parse(b'\x86' + count.to_bytes(3, 'big') + b'x' * 17)
        self.assertEqual((code, out[1]), (0, 21))
        count -= 17
        while count:
            n = min(count, 4096)
            code, out = self.parse(b'x' * n)
            self.assertEqual((code, out[1]), (0, n))
            count -= n
        _, frames = self.packets('tone')
        self.assertEqual(self.parse(frames[0], eos=True)[1][0], len(frames[0]))

    def test_native_bounds_and_fragmented_stream(self):
        data, _ = self.packets('noise')
        base = Path(self.tmp.name)
        source = base / 'bounds.c'
        fixture = base / 'bounds.flac'; fixture.write_bytes(data)
        source.write_text(r'''
#include "streamoo_flac_parse.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(int argc,char **argv){
 assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
 const size_t cap=128*1024+16;uint8_t *cache=malloc(cap);assert(cache);
 for(unsigned chunk=1;chunk<=4096;chunk=chunk==1?257:4096){
  rewind(f);esp_es_parse_frame_info_t info={0};size_t n=0;bool bos=true,eof=false;
  unsigned frames=0;
  while(!eof||n){
   if(!eof){size_t take=chunk;if(take>cap-n)take=cap-n;assert(take);
    size_t got=fread(cache+n,1,take,f);n+=got;eof=got<take;}
   while(n){
    esp_es_parse_raw_t in={.buffer=cache,.len=(uint32_t)n,.bos=bos,.eos=eof};
    int r=streamoo_flac_parse(&in,&info);
    if(r==ESP_ES_PARSE_ERR_DATA_NOT_ENOUGH||info.frame_size==ES_PARSE_REPARSE_FRAME_SIZE){assert(!eof);break;}
    assert(r==0);size_t used=info.skipped_size?info.skipped_size:info.frame_size;
    assert(used&&used<=n);if(info.skipped_size)bos=false;else ++frames;
    memmove(cache,cache+used,n-used);n-=used;
   }
  }
  assert(frames==8);free(info.extra_data);if(chunk==4096)break;
 }
 fclose(f);free(cache);
 /* Arbitrary short prefixes must stay inside the supplied buffer. */
 unsigned seed=17;
 for(unsigned trial=0;trial<4000;++trial){
  size_t n=trial%80;uint8_t *p=malloc(n?n:1);assert(p);
  for(size_t i=0;i<n;++i){seed=seed*1664525u+1013904223u;p[i]=(uint8_t)(seed>>24);}
  esp_es_parse_frame_info_t info={0};
  esp_es_parse_raw_t in={.buffer=p,.len=(uint32_t)n,.bos=true,.eos=true};
  streamoo_flac_parse(&in,&info);free(info.extra_data);free(p);
 }
 return 0;
}
''')
        binary = base / 'bounds'
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                        *os.environ.get('STREAMOO_FLAC_SANITIZE', '').split(),
                        '-Icomponents/decoder',
                        '-Imanaged_components/espressif__esp_audio_codec/include/simple_dec',
                        str(source), 'components/decoder/streamoo_flac_parse.c',
                        'components/decoder/streamoo_flac_header.c', '-o', str(binary)],
                       cwd=ROOT, check=True)
        subprocess.run([str(binary), str(fixture)], check=True)
