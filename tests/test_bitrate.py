import ctypes as C
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]


class State(C.Structure):
    _fields_=[('consumed',C.c_uint64),('produced',C.c_uint64),('rate',C.c_uint32),
              ('kbps',C.c_uint32),('channels',C.c_uint8),('bits',C.c_uint8),('primed',C.c_bool)]


class BitrateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup)
        base=Path(cls.tmp.name)
        (base/'test.c').write_text('#include "streamoo_bitrate.h"\nuint32_t update(streamoo_bitrate_t *s,uint64_t a,uint64_t b,uint32_t r,uint8_t c,uint8_t d){return streamoo_bitrate_update(s,a,b,r,c,d);}\n')
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',
                        '-I'+str(ROOT/'components/player/include'),str(base/'test.c'),'-o',str(base/'test.so')],check=True)
        cls.lib=C.CDLL(str(base/'test.so'))
        cls.lib.update.argtypes=[C.POINTER(State),C.c_uint64,C.c_uint64,C.c_uint32,C.c_uint8,C.c_uint8]
        cls.lib.update.restype=C.c_uint32

    def update(self,s,a,b,rate=44100,channels=2,bits=32):
        return self.lib.update(C.byref(s),a,b,rate,channels,bits)

    def test_decoded_duration_all_widths_and_rates_excludes_initial_metadata(self):
        for rate in (8000,44100,48000):
            for channels in (1,2):
                for bits in (16,24,32):
                    with self.subTest(rate=rate,channels=channels,bits=bits):
                        s=State();frame=channels*bits//8;first=4608*frame
                        self.assertEqual(self.update(s,90000,first,rate,channels,bits),0)
                        self.assertEqual(self.update(s,90000+120000,first+rate*frame*2,rate,channels,bits),480)

    def test_waits_do_not_change_value_and_new_window_tracks_vbr(self):
        s=State();self.update(s,20000,36864)
        self.assertEqual(self.update(s,20000+10000,36864+44100*8),0)
        self.assertEqual(self.update(s,20000+450000,36864+44100*8*2),1800)
        self.assertEqual(self.update(s,s.consumed,s.produced),1800)
        self.assertEqual(self.update(s,s.consumed+50000,s.produced+44100*8*2),200)

    def test_format_change_counter_reset_and_invalid_input_clear_estimate(self):
        s=State();self.update(s,1000,36864)
        self.update(s,101000,36864+44100*8*2)
        self.assertEqual(self.update(s,101001,s.produced+1,48000),0)
        self.assertEqual(self.update(s,1,1,48000),0)
        for rate,channels,bits in ((0,2,16),(96000,2,16),(44100,0,16),(44100,3,16),(44100,2,8)):
            self.assertEqual(self.update(s,1,1,rate,channels,bits),0)
            self.assertFalse(s.primed)
        self.assertEqual(self.lib.update(None,1,1,44100,2,32),0)

    def test_empty_pcm_does_not_prime_and_huge_deltas_do_not_overflow(self):
        s=State();self.assertEqual(self.update(s,9000,0),0);self.assertFalse(s.primed)
        self.update(s,10000,36864)
        self.assertEqual(self.update(s,2**64-1,2**64-1),0)
