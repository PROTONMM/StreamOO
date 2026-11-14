"""Actual portable C FFT compared with independent NumPy rFFT (if installed)."""
import ctypes as C
import math
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
try:
    import numpy as np
except ImportError:
    np = None
ROOT=Path(__file__).resolve().parents[1]
class Spectrum(C.Structure):
    _fields_=[('size',C.c_uint32),('sample_rate',C.c_uint32),('window_sum',C.c_float),
        ('window',C.c_float*2048),('twiddle',C.c_float*2048),('work',C.c_float*4096),
        ('amplitude',C.c_float*1025),('edges',C.c_uint16*25),('bands',C.c_uint16*24)]
class SpectrumTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.temp.cleanup)
        lib=Path(cls.temp.name)/'spectrum.so'
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',
            '-Icomponents/spectrum/include','components/spectrum/streamoo_spectrum.c',
            '-lm','-o',str(lib)],cwd=ROOT,check=True)
        cls.lib=C.CDLL(str(lib))
        cls.init=cls.lib.streamoo_spectrum_init
        cls.init.argtypes=[C.POINTER(Spectrum),C.c_uint32,C.c_uint32];cls.init.restype=C.c_bool
        cls.analyze=cls.lib.streamoo_spectrum_analyze
        cls.analyze.argtypes=[C.POINTER(Spectrum),C.POINTER(C.c_int32),C.c_size_t];cls.analyze.restype=C.c_bool
    def context(self,n=1024,rate=48000):
        s=Spectrum();self.assertTrue(self.init(C.byref(s),n,rate));return s
    def signal(self,s,frequency,phase=-1,one_channel=False):
        lr=(C.c_int32*(s.size*2))()
        for i in range(s.size):
            value=round(.75*math.sin(2*math.pi*frequency*i/s.sample_rate)*2147483647)
            lr[2*i]=value;lr[2*i+1]=0 if one_channel else value*phase
        return lr
    def test_bad_input_does_not_write(self):
        s=self.context();before=bytes(s);lr=self.signal(s,1000)
        for n,rate in ((0,48000),(512,48000),(4096,48000),(1024,7999),(1024,48001)):
            self.assertFalse(self.init(C.byref(s),n,rate));self.assertEqual(bytes(s),before)
        self.assertFalse(self.analyze(C.byref(s),lr,1023));self.assertEqual(bytes(s),before)
        self.assertFalse(self.analyze(C.byref(s),None,1024));self.assertEqual(bytes(s),before)
        self.assertFalse(self.init(None,1024,48000))
    def test_silence_and_dc_removal(self):
        for n in (1024,2048):
            s=self.context(n)
            for value in (0,-2147483648,2147483647):
                lr=(C.c_int32*(2*n))(*([value]*(2*n)))
                self.assertTrue(self.analyze(C.byref(s),lr,n))
                self.assertEqual(max(s.amplitude),0);self.assertEqual(max(s.bands),0)
    def test_hann_symmetric_n_minus_one_and_q15(self):
        for n in (1024,2048):
            s=self.context(n);self.assertEqual(s.window[0],0);self.assertEqual(s.window[n-1],0)
            self.assertAlmostEqual(s.window_sum,(n-1)/2,places=3)
            for i in range(n):
                expected=.5*(1-math.cos(2*math.pi*i/(n-1)))
                self.assertAlmostEqual(s.window[i],expected,places=6)
                self.assertEqual(s.window[i],s.window[n-1-i])
                q15=round(s.window[i]*32767)
                self.assertLessEqual(abs(q15/32767-expected),1/32767)
    def test_all_bins_covered_once_with_nonempty_log_bands(self):
        for n in (1024,2048):
            s=self.context(n);edges=list(s.edges)
            self.assertEqual(edges[0],1);self.assertEqual(edges[-1],n//2+1)
            self.assertTrue(all(a<b for a,b in zip(edges,edges[1:])))
            covered=[k for a,b in zip(edges,edges[1:]) for k in range(a,b)]
            self.assertEqual(covered,list(range(1,n//2+1)))
    def test_coherent_gain_stereo_antiphase_and_single_channel(self):
        for n in (1024,2048):
            s=self.context(n);f=20*48000/n
            for phase,one,gain in ((1,False,1),(-1,False,1),(1,True,math.sqrt(.5))):
                lr=self.signal(s,f,phase,one);before=bytes(lr)
                self.assertTrue(self.analyze(C.byref(s),lr,n));self.assertEqual(bytes(lr),before)
                self.assertAlmostEqual(s.amplitude[20],.75*gain,places=5)
                self.assertLessEqual(max(s.bands),32767)
    def test_nyquist_not_doubled(self):
        s=self.context();lr=(C.c_int32*2048)()
        for i in range(1024):lr[2*i]=lr[2*i+1]=1073741824 if i%2 else -1073741824
        self.assertTrue(self.analyze(C.byref(s),lr,1024));self.assertAlmostEqual(s.amplitude[512],.5,places=6)
    @unittest.skipIf(np is None,'Optional NumPy oracle: install numpy to compare complete rFFT spectra')
    def test_numpy_reference_tones_and_sample_rates(self):
        for n in (1024,2048):
            for rate in (8000,44100,48000):
                for f in (100,1000,min(8000,rate/3)):
                    s=self.context(n,rate);lr=self.signal(s,f)
                    self.compare_numpy(s,lr)
    @unittest.skipIf(np is None,'Optional NumPy oracle')
    def test_numpy_reference_impulse_and_full_scale_noise(self):
        rng=np.random.default_rng(670)
        for n in (1024,2048):
            s=self.context(n);lr=(C.c_int32*(2*n))();lr[0]=-2147483648
            self.compare_numpy(s,lr)
            values=rng.integers(-2147483648,2147483647,size=2*n,dtype=np.int32)
            self.compare_numpy(s,(C.c_int32*(2*n))(*values))
    def test_native_bounds_and_actual_pre_gain_tap_at_three_volumes(self):
        binary=str(Path(self.temp.name)/'spectrum_native')
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',
            *os.environ.get('STREAMOO_SPECTRUM_SANITIZE','').split(),
            '-Icomponents/spectrum/include','-Icomponents/pcm/include','-Icomponents/audio_gain/include',
            'components/spectrum/streamoo_spectrum.c','components/pcm/streamoo_pcm.c',
            'components/audio_gain/streamoo_audio_gain.c','tests/spectrum_test.c','-lm','-o',binary],cwd=ROOT,check=True)
        subprocess.run([binary],check=True)
    @unittest.skipIf(np is None,'Optional NumPy rectangular-window comparison')
    def test_hann_reduces_far_leakage_for_off_bin_tone(self):
        for n in (1024,2048):
            s=self.context(n);frequency=20.37*48000/n
            lr=self.signal(s,frequency,phase=1)
            self.assertTrue(self.analyze(C.byref(s),lr,n))
            values=np.ctypeslib.as_array(lr)[::2].astype(np.float64)/2147483648
            values-=values.mean()
            rectangle=np.abs(np.fft.rfft(values))*2/n
            hann=np.array(s.amplitude[:n//2+1])
            far=np.abs(np.arange(n//2+1)-20.37)>=8
            far[0]=False
            self.assertLess(max(hann[far]),max(rectangle[far])/50)
    def compare_numpy(self,s,lr):
        self.assertTrue(self.analyze(C.byref(s),lr,s.size))
        values=np.ctypeslib.as_array(lr).reshape(-1,2).astype(np.float64)/2147483648
        w=np.hanning(s.size)
        values-=(values*w[:,None]).sum(axis=0)/w.sum()
        z=np.fft.rfft(values*w[:,None],axis=0)
        scale=np.full(s.size//2+1,2/w.sum());scale[-1]=1/w.sum()
        expected=np.sqrt(np.mean(np.abs(z*scale[:,None])**2,axis=1));expected[0]=0
        actual=np.array(s.amplitude[:s.size//2+1])
        np.testing.assert_allclose(actual,expected,atol=2e-6,rtol=2e-4)
        for b in range(24):
            value=min(32767,round(max(expected[s.edges[b]:s.edges[b+1]])*32767))
            self.assertLessEqual(abs(s.bands[b]-value),1)
