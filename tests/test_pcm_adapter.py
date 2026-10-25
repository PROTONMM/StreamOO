from pathlib import Path
import ctypes as C
import hashlib
import json
import struct
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class Format(C.Structure):
    _fields_=[('rate',C.c_uint32),('channels',C.c_uint8),('bits',C.c_uint8)]
class Clip(C.Structure):
    _fields_=[('format',Format),('data',C.POINTER(C.c_uint8)),('bytes',C.c_size_t),('cursor',C.c_size_t)]
class PCMAdapterTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory(); cls.addClassCleanup(cls.temp.cleanup)
        lib=Path(cls.temp.name)/'adapter.so'
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',
            '-Icomponents/pcm_adapter/include','components/pcm_adapter/streamoo_pcm_adapter.c',
            '-o',str(lib)],cwd=ROOT,check=True)
        cls.lib=C.CDLL(str(lib))
        cls.convert=cls.lib.streamoo_pcm16le_convert
        cls.lib.streamoo_pcm_clip_init.argtypes=[C.POINTER(Clip),Format,C.POINTER(C.c_uint8),C.c_size_t]
        cls.lib.streamoo_pcm_clip_init.restype=C.c_bool
        cls.lib.streamoo_pcm_clip_read.argtypes=[C.POINTER(Clip),C.POINTER(C.c_int32),C.c_size_t]
        cls.lib.streamoo_pcm_clip_read.restype=C.c_bool
        cls.convert.argtypes=[Format,C.POINTER(C.c_uint8),C.c_size_t,C.POINTER(C.c_int32),C.c_size_t]
        cls.convert.restype=C.c_bool
    def test_stereo_endpoints_order_and_sign(self):
        data=struct.pack('<hhhh',-32768,32767,1,-1)
        src=(C.c_uint8*len(data)).from_buffer_copy(data); out=(C.c_int32*4)()
        self.assertTrue(self.convert(Format(48000,2,16),src,8,out,2))
        self.assertEqual(list(out),[-2147483648,2147418112,65536,-65536])
    def test_all_s16_values_mono(self):
        for start in range(-32768,32768,240):
            values=list(range(start,min(start+240,32768)))
            data=struct.pack('<'+'h'*len(values),*values)
            src=(C.c_uint8*len(data)).from_buffer_copy(data); out=(C.c_int32*(len(values)*2))()
            self.assertTrue(self.convert(Format(48000,1,16),src,len(data),out,len(values)))
            self.assertEqual(list(out),[word for v in values for word in (v*65536,v*65536)])
    def test_unaligned_source(self):
        src=(C.c_uint8*5)(99,0,128,255,127)
        pointer=C.cast(C.byref(src,1),C.POINTER(C.c_uint8)); out=(C.c_int32*2)()
        self.assertTrue(self.convert(Format(48000,2,16),pointer,4,out,1))
        self.assertEqual(list(out),[-2147483648,2147418112])
    def test_unsupported_formats_leave_output(self):
        src=(C.c_uint8*4)(); out=(C.c_int32*2)(17,19)
        for rate,channels,bits in ((44100,2,16),(48000,0,16),(48000,3,16),(48000,2,24)):
            self.assertFalse(self.convert(Format(rate,channels,bits),src,4,out,1))
            self.assertEqual(list(out),[17,19])
    def test_bounds_and_partial_frames(self):
        src=(C.c_uint8*960)(); out=(C.c_int32*482)(*([77]*482))
        self.assertFalse(self.convert(Format(48000,2,16),src,959,out,240))
        self.assertFalse(self.convert(Format(48000,2,16),src,964,out,241))
        self.assertFalse(self.convert(Format(48000,2,16),src,0,out,C.c_size_t(-1).value))
        ptr=C.cast(C.byref(out,4),C.POINTER(C.c_int32))
        self.assertTrue(self.convert(Format(48000,2,16),src,960,ptr,240))
        self.assertEqual(out[0],77); self.assertEqual(out[-1],77)
    def test_fixture_integrity_and_mpeg_frames(self):
        manifest=json.loads((ROOT/'assets/audio/channels_48k_stereo.json').read_text())
        data=(ROOT/'assets/audio/channels_48k_stereo.mp3').read_bytes()
        self.assertEqual(hashlib.sha256(data).hexdigest(),manifest['mp3_sha256'])
        self.assertEqual(len(data),manifest['mp3_bytes'])
        offset=frames=0
        while offset<len(data):
            h=int.from_bytes(data[offset:offset+4],'big')
            self.assertEqual(h>>21,0x7ff); self.assertEqual((h>>19)&3,3)
            self.assertEqual((h>>17)&3,1); self.assertEqual((h>>12)&15,9)
            self.assertEqual((h>>10)&3,1); self.assertNotEqual((h>>6)&3,3)
            offset+=384+((h>>9)&1); frames+=1
        self.assertEqual(offset,len(data)); self.assertEqual(frames*1152,manifest['reference_frames'])

    def test_clip_wrap_across_stereo_boundary(self):
        data=struct.pack('<hhhhhh',1,-1,2,-2,3,-3)
        src=(C.c_uint8*len(data)).from_buffer_copy(data); clip=Clip(); out=(C.c_int32*8)()
        self.assertTrue(self.lib.streamoo_pcm_clip_init(C.byref(clip),Format(48000,2,16),src,len(data)))
        clip.cursor=8
        self.assertTrue(self.lib.streamoo_pcm_clip_read(C.byref(clip),out,4))
        self.assertEqual(list(out),[v*65536 for v in (3,-3,1,-1,2,-2,3,-3)])
        self.assertEqual(clip.cursor,0)
    def test_one_frame_clip_repeats_full_block(self):
        src=(C.c_uint8*2)(0,128); clip=Clip(); out=(C.c_int32*480)()
        self.assertTrue(self.lib.streamoo_pcm_clip_init(C.byref(clip),Format(48000,1,16),src,2))
        self.assertTrue(self.lib.streamoo_pcm_clip_read(C.byref(clip),out,240))
        self.assertTrue(all(v==-2147483648 for v in out)); self.assertEqual(clip.cursor,0)
    def test_clip_invalid_data_and_cursor_atomic(self):
        src=(C.c_uint8*4)(); clip=Clip(); out=(C.c_int32*2)(17,19)
        self.assertFalse(self.lib.streamoo_pcm_clip_init(C.byref(clip),Format(48000,2,16),src,3))
        self.assertTrue(self.lib.streamoo_pcm_clip_init(C.byref(clip),Format(48000,2,16),src,4))
        clip.cursor=1; before=bytes(clip)
        self.assertFalse(self.lib.streamoo_pcm_clip_read(C.byref(clip),out,1))
        self.assertEqual(bytes(clip),before); self.assertEqual(list(out),[17,19])
