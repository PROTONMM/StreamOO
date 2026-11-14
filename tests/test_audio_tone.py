from pathlib import Path
import array
import ctypes as C
import math
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
class Tone(C.Structure):
    _fields_ = [('position', C.c_uint32)]

class AudioToneTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        library = str(Path(cls.temp.name) / 'tone.so')
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',
                        '-Imain','main/audio_tone.c','-o',library],cwd=ROOT,check=True)
        cls.lib = C.CDLL(library)
        cls.lib.streamoo_audio_tone_fill.argtypes = [C.POINTER(Tone), C.POINTER(C.c_int32), C.c_size_t]
        cls.lib.streamoo_audio_tone_fill.restype = C.c_bool
        cls.lib.streamoo_audio_sine_fill.argtypes = [C.POINTER(C.c_uint32), C.POINTER(C.c_int32), C.c_size_t]
        cls.lib.streamoo_audio_sine_fill.restype = C.c_bool

    def render(self, start, count, chunk=240):
        tone = Tone(start)
        result = array.array('i')
        while count:
            frames = min(count,chunk)
            samples = (C.c_int32 * (frames*2))()
            self.assertTrue(self.lib.streamoo_audio_tone_fill(C.byref(tone),samples,frames))
            result.frombytes(bytes(samples))
            count -= frames
        return result, tone.position

    def test_silent_intervals_and_finite_end(self):
        for start in (0,144000,288000,432000,480000,0xffffffff):
            samples, _ = self.render(start,4800)
            self.assertFalse(any(samples))
        _, end = self.render(479999,1000)
        self.assertEqual(end,480000)

    def test_channel_isolation_and_stereo(self):
        left,_ = self.render(60000,2400)
        right,_ = self.render(204000,2400)
        both,_ = self.render(348000,2400)
        self.assertTrue(any(left[::2])); self.assertFalse(any(left[1::2]))
        self.assertFalse(any(right[::2])); self.assertTrue(any(right[1::2]))
        self.assertEqual(both[::2],both[1::2])

    def test_frequencies(self):
        for start,channel,hz in ((60000,0,1000),(204000,1,2000)):
            samples,_ = self.render(start,48000)
            values = samples[channel::2]
            crossings = sum(values[i-1]<=0<values[i] for i in range(1,len(values)))
            self.assertAlmostEqual(crossings,hz,delta=1)

    def test_amplitude_rms_and_dc(self):
        samples,_ = self.render(60000,4800)
        values = [v//65536 for v in samples[::2]]
        self.assertEqual(max(values),1036); self.assertEqual(min(values),-1036)
        self.assertAlmostEqual(sum(values)/len(values),0,places=5)
        rms = math.sqrt(sum(v*v for v in values)/len(values))
        self.assertAlmostEqual(rms,1036/math.sqrt(2),delta=1)
        self.assertAlmostEqual(20*math.log10(1036/32768),-30,delta=.01)

    def test_ramp_boundaries(self):
        for start in (48000,192000,336000):
            samples,_ = self.render(start,2400)
            self.assertEqual(samples[0],0); self.assertEqual(samples[1],0)
            beginning = max(abs(v) for v in samples[:480])
            ending = max(abs(v) for v in samples[-480:])
            self.assertLess(beginning,ending//5)
        for end in (144000,288000,432000):
            samples,_ = self.render(end-1,2)
            self.assertFalse(any(samples))

    def test_chunk_invariance_and_canaries(self):
        expected,end = self.render(47900,10000)
        for chunk in (1,37,137,240):
            actual,actual_end = self.render(47900,10000,chunk)
            self.assertEqual(actual,expected); self.assertEqual(actual_end,end)
        storage=(C.c_int32*482)(*([123456]*482))
        ptr=C.cast(C.byref(storage,4),C.POINTER(C.c_int32))
        tone=Tone(60000)
        self.assertTrue(self.lib.streamoo_audio_tone_fill(C.byref(tone),ptr,240))
        self.assertEqual(storage[0],123456); self.assertEqual(storage[-1],123456)

    def test_invalid_fill_is_atomic(self):
        tone=Tone(123)
        storage=(C.c_int32*2)(17,19)
        self.assertFalse(self.lib.streamoo_audio_tone_fill(C.byref(tone),storage,241))
        self.assertFalse(self.lib.streamoo_audio_tone_fill(None,storage,1))
        self.assertFalse(self.lib.streamoo_audio_tone_fill(C.byref(tone),None,1))
        self.assertEqual(tone.position,123); self.assertEqual(list(storage),[17,19])

    def test_entire_sequence_has_bounded_packed_samples(self):
        samples,end = self.render(0,480000)
        self.assertEqual(end,480000)
        self.assertLessEqual(max(abs(v) for v in samples),1036*65536)
        self.assertTrue(all(v%65536==0 for v in samples))

    def test_continuous_source_phase_and_invalid_state(self):
        phase=C.c_uint32(0); samples=(C.c_int32*192)()
        self.assertTrue(self.lib.streamoo_audio_sine_fill(C.byref(phase),samples,96))
        self.assertEqual(phase.value,0)
        self.assertEqual(list(samples[:96]),list(samples[96:]))
        self.assertEqual(list(samples[::2]),list(samples[1::2]))
        self.assertEqual(max(samples),1036*65536)
        phase.value=48; old=bytes(samples)
        self.assertFalse(self.lib.streamoo_audio_sine_fill(C.byref(phase),samples,96))
        self.assertEqual(bytes(samples),old); self.assertEqual(phase.value,48)
