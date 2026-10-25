from pathlib import Path
import ctypes as C
import math
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
class Gain(C.Structure):
    _fields_=[(name,C.c_int32) for name in ('current','target','step','remainder','error')]+[('remaining',C.c_uint32)]

class AudioGainTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory(); cls.addClassCleanup(cls.temp.cleanup)
        lib=Path(cls.temp.name)/'gain.so'
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',
            '-Icomponents/audio_gain/include','components/audio_gain/streamoo_audio_gain.c',
            '-o',str(lib)],cwd=ROOT,check=True)
        cls.lib=C.CDLL(str(lib))
        cls.lib.streamoo_audio_gain_volume.argtypes=[C.c_uint8,C.POINTER(C.c_uint32)]
        cls.lib.streamoo_audio_gain_volume.restype=C.c_bool
        cls.lib.streamoo_audio_gain_target.argtypes=[C.POINTER(Gain),C.c_uint32]
        cls.lib.streamoo_audio_gain_target.restype=C.c_bool
        cls.lib.streamoo_audio_gain_process.argtypes=[C.POINTER(Gain),C.POINTER(C.c_int32),C.c_size_t]
        cls.lib.streamoo_audio_gain_process.restype=C.c_bool

    def target(self,s,value):
        self.assertTrue(self.lib.streamoo_audio_gain_target(C.byref(s),value))
    def frame(self,s,left=2147483647,right=-2147483648):
        b=(C.c_int32*2)(left,right)
        self.assertTrue(self.lib.streamoo_audio_gain_process(C.byref(s),b,1))
        return list(b)
    def settle(self,s):
        for _ in range(2400): self.frame(s)

    def test_curve_all_values_and_endpoints(self):
        previous=-1
        for volume in range(101):
            value=C.c_uint32()
            self.assertTrue(self.lib.streamoo_audio_gain_volume(volume,C.byref(value)))
            expected=0 if volume==0 else round(65536*10**((60*(volume/100-1))/20))
            self.assertEqual(value.value,expected)
            self.assertGreater(value.value,previous); previous=value.value
        self.assertEqual(previous,65536)

    def test_ramp_exact_duration_and_monotonic(self):
        s=Gain(); self.target(s,65536)
        last=0
        for i in range(2400):
            self.frame(s)
            self.assertGreaterEqual(s.current,last)
            self.assertLessEqual(s.current-last,28)
            self.assertEqual(s.remaining,2399-i)
            last=s.current
        self.assertEqual(s.current,65536)
        self.target(s,0)
        for _ in range(2400):
            self.frame(s); self.assertLessEqual(s.current,last); last=s.current
        self.assertEqual(s.current,0)
        self.assertEqual(self.frame(s),[0,0])

    def test_unity_preserves_extremes_and_stereo(self):
        s=Gain(); self.target(s,65536); self.settle(s)
        self.assertEqual(self.frame(s),[2147483647,-2147483648])
        for value in (-2147483648,-123456,0,123456,2147483647):
            self.assertEqual(self.frame(s,value,value),[value,value])

    def test_retarget_is_continuous_and_duplicate_does_not_restart(self):
        s=Gain(); self.target(s,65536)
        for _ in range(333): self.frame(s)
        before=s.current; left=s.remaining
        self.target(s,65536); self.assertEqual(s.remaining,left)
        self.target(s,0); self.assertEqual(s.current,before)
        self.frame(s); self.assertLessEqual(abs(s.current-before),28)
        for _ in range(2399): self.frame(s)
        self.assertEqual(s.current,0)

    def test_chunk_invariance(self):
        def render(chunk):
            s=Gain(); self.target(s,12345); out=[]
            remaining=4800
            while remaining:
                n=min(chunk,remaining); b=(C.c_int32*(2*n))(*([1000000,-1000000]*n))
                self.assertTrue(self.lib.streamoo_audio_gain_process(C.byref(s),b,n))
                out.extend(b); remaining-=n
            return out,bytes(s)
        expected=render(1)
        for chunk in (37,137,240): self.assertEqual(render(chunk),expected)

    def test_no_clipping_and_zero_dc_for_bipolar_pair(self):
        s=Gain()
        for target in (1,71,1000,2072,32768,65536,0):
            self.target(s,target)
            for _ in range(2400):
                l,r=self.frame(s,2147483647,-2147483647)
                self.assertEqual(l,-r)
                self.assertLessEqual(abs(l),2147483647)
                self.assertGreaterEqual(s.current,0); self.assertLessEqual(s.current,65536)

    def test_invalid_inputs_preserve_state_and_canaries(self):
        s=Gain(); old=bytes(s)
        self.assertFalse(self.lib.streamoo_audio_gain_target(C.byref(s),65537))
        b=(C.c_int32*482)(*([123]*482)); ptr=C.cast(C.byref(b,4),C.POINTER(C.c_int32))
        self.assertFalse(self.lib.streamoo_audio_gain_process(C.byref(s),ptr,241))
        self.assertEqual(bytes(s),old); self.assertTrue(all(v==123 for v in b))
        self.target(s,65536)
        self.assertTrue(self.lib.streamoo_audio_gain_process(C.byref(s),ptr,240))
        self.assertEqual(b[0],123); self.assertEqual(b[-1],123)
