"""Compare the actual ESP-DSP sc16 ANSI oracle with StreamOO float/NumPy.

The host aliases only the AES3 call to the library's ANSI implementation.
It tests scaling, headroom and ordering; target instruction correctness and
execution time must be checked on ESP32-S3. No vendor implementation is copied.
"""
import ctypes as C
import math
import os
from pathlib import Path
import random
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
DSP=Path(os.environ.get('STREAMOO_ESP_DSP_PATH',
                         ROOT/'managed_components/espressif__esp-dsp'))
try:
    import numpy as np
except ImportError:
    np=None

class SpectrumSc16Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not (DSP/'modules/fft/fixed/dsps_fft2r_sc16_ansi.c').is_file():
            raise unittest.SkipTest('ESP-DSP source missing: run idf.py reconfigure or set STREAMOO_ESP_DSP_PATH')
        cls.temp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.temp.cleanup)
        base=Path(cls.temp.name)
        (base/'sdkconfig.h').write_text('#pragma once\n#define CONFIG_STREAMOO_DSP_ENABLE_SIMD 1\n#define CONFIG_DSP_MAX_FFT_SIZE 2048\n')
        # FFT source includes this test-only umbrella but uses no symbols in it.
        (base/'dsp_tests.h').write_text('#pragma once\n')
        (base/'esp_idf_version.h').write_text('#pragma once\n#define ESP_IDF_VERSION_VAL(a,b,c) (((a)<<16)|((b)<<8)|(c))\n#define ESP_IDF_VERSION ESP_IDF_VERSION_VAL(6,1,0)\n')
        (base/'esp_log.h').write_text('#pragma once\nstatic inline void host_dsp_log(const char *tag,const char *fmt,...){(void)tag;(void)fmt;}\n#define ESP_LOGD host_dsp_log\n')
        cls.flags=['-Wno-unused-parameter','-I'+str(base),'-Icomponents/spectrum/include',
                   '-I'+str(DSP/'modules/fft/include'),
                   '-I'+str(DSP/'modules/common/include'),
                   '-I'+str(DSP/'modules/common/include_sim')]
        cls.sources=['components/spectrum/streamoo_spectrum.c','tests/spectrum_backend_bridge.c']
        cls.vendor=[str(DSP/'modules/fft/fixed/dsps_fft2r_sc16_ansi.c'),
                    str(DSP/'modules/fft/float/dsps_fft2r_fc32_ansi.c'),
                    str(DSP/'modules/fft/float/dsps_fft2r_bitrev_tables_fc32.c'),
                    '-x','c',str(DSP/'modules/common/misc/dsps_pwroftwo.cpp')]
        cls.sc16flags=['-DESP_PLATFORM','-Ddsps_fft2r_sc16_aes3_=dsps_fft2r_sc16_ansi_']
        cls.libs=[]
        for backend in ('reference','sc16'):
            lib=base/(backend+'.so')
            subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',
                *cls.flags,*(cls.sc16flags if backend=='sc16' else []),*cls.sources,
                *(cls.vendor if backend=='sc16' else []),'-lm','-o',str(lib)],cwd=ROOT,check=True)
            api=C.CDLL(str(lib))
            api.spectrum_create.argtypes=[C.c_uint,C.c_uint];api.spectrum_create.restype=C.c_void_p
            api.spectrum_destroy.argtypes=[C.c_void_p]
            api.spectrum_run.argtypes=[C.c_void_p,C.POINTER(C.c_int32),C.c_size_t];api.spectrum_run.restype=C.c_bool
            api.spectrum_amplitudes.argtypes=[C.c_void_p];api.spectrum_amplitudes.restype=C.POINTER(C.c_float)
            api.spectrum_bands.argtypes=[C.c_void_p];api.spectrum_bands.restype=C.POINTER(C.c_uint16)
            api.spectrum_workspace_bytes.restype=C.c_size_t
            cls.libs.append(api)

    def spectra(self,n,values,rate=48000):
        samples=(C.c_int32*(n*2))(*values);before=bytes(samples);results=[]
        for lib in self.libs:
            ctx=lib.spectrum_create(n,rate);self.assertTrue(ctx)
            try:
                self.assertTrue(lib.spectrum_run(ctx,samples,n))
                self.assertFalse(lib.spectrum_run(ctx,samples,n-1))
                self.assertFalse(lib.spectrum_run(ctx,None,n))
                results.append(([lib.spectrum_amplitudes(ctx)[i] for i in range(n//2+1)],
                                [lib.spectrum_bands(ctx)[i] for i in range(24)]))
            finally:
                lib.spectrum_destroy(ctx)
        self.assertEqual(bytes(samples),before)
        return results

    def compare(self,n,values,rate=48000):
        reference,sc16=self.spectra(n,values,rate)
        # Absolute amplitude tolerance includes stage-by-stage Q15 rounding.
        self.assertLess(max(abs(a-b) for a,b in zip(reference[0],sc16[0])),0.0025)
        self.assertLessEqual(max(abs(a-b) for a,b in zip(reference[1],sc16[1])),82)
        return reference,sc16

    def test_tones_off_bin_rates_stereo_phase_and_quiet_input(self):
        for n in (1024,2048):
            for rate in (8000,44100,48000):
                for amplitude in (0.0001,0.1,0.99):
                    for phase in (0,1,-1):
                        values=[]
                        for i in range(n):
                            value=round(amplitude*math.sin(2*math.pi*20.37*i/n)*2147483647)
                            values.extend((value,phase*value))
                        self.compare(n,values,rate)

    def test_silence_extreme_dc_and_nyquist(self):
        for n in (1024,2048):
            for value in (0,-2147483648,2147483647):
                for amplitudes,bands in self.spectra(n,[value]*(2*n)):
                    self.assertEqual(max(amplitudes),0);self.assertEqual(max(bands),0)
            values=[int(1073741824 if i%2 else -1073741824) for i in range(n) for _ in range(2)]
            _,sc16=self.compare(n,values)
            self.assertAlmostEqual(sc16[0][n//2],0.5,delta=0.0025)

    def test_full_scale_noise_impulse_and_dc_offset_transients(self):
        rng=random.Random(670)
        for n in (1024,2048):
            self.compare(n,[rng.randrange(-2147483648,2147483648) for _ in range(n*2)])
            values=[2147483647]*(n*2);values[n]=-2147483648
            self.compare(n,values)
            values=[0]*(n*2);values[n]=-2147483648
            self.compare(n,values)

    def test_coherent_tone_keeps_calibration_across_sc16_scaling(self):
        for amplitude in (0.0001,0.01,0.5,0.99):
            values=[round(amplitude*math.sin(2*math.pi*20*i/1024)*2147483647)
                    for i in range(1024) for _ in range(2)]
            _,sc16=self.compare(1024,values)
            self.assertAlmostEqual(sc16[0][20]/amplitude,1,delta=0.005)

    def test_workspace_smaller_and_multiple_sizes_share_table(self):
        self.assertLess(self.libs[1].spectrum_workspace_bytes(),self.libs[0].spectrum_workspace_bytes())
        for n in (2048,1024,2048,1024):
            values=[round(0.75*math.sin(2*math.pi*20*i/n)*2147483647)
                    for i in range(n) for _ in range(2)]
            self.compare(n,values)

    def test_alignment_and_global_table_ownership_fail_cleanly(self):
        check=self.libs[1].spectrum_rejects_misalignment_and_table_conflict
        check.restype=C.c_bool
        self.assertTrue(check())
        # Deinit in the experiment must allow the normal adapter to reinit.
        self.compare(1024,[0]*2048)

    @unittest.skipIf(np is None,'NumPy oracle unavailable')
    def test_numpy_independent_calibration_and_band_input(self):
        for n in (1024,2048):
            values=[round(0.9*math.sin(2*math.pi*20.37*i/n)*2147483647)
                    for i in range(n) for _ in range(2)]
            _,sc16=self.spectra(n,values)
            x=np.array(values,dtype=np.float64).reshape(n,2)/2147483648
            w=np.hanning(n);x-=(x*w[:,None]).sum(axis=0)/w.sum()
            z=np.fft.rfft(x*w[:,None],axis=0)
            scale=np.full(n//2+1,2/w.sum());scale[-1]=1/w.sum()
            expected=np.sqrt(np.mean(np.abs(z*scale[:,None])**2,axis=1));expected[0]=0
            np.testing.assert_allclose(sc16[0],expected,atol=0.0025,rtol=0.005)

    def test_native_bounds_and_sanitizers(self):
        binary=str(Path(self.temp.name)/'sc16_native')
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',
            *os.environ.get('STREAMOO_SC16_SANITIZE','').split(),*self.flags,*self.sc16flags,
            '-DSTREAMOO_SC16_HOST_TEST',*self.sources,*self.vendor,'-lm','-o',binary],cwd=ROOT,check=True)
        subprocess.run([binary],check=True)
