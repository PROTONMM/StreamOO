from pathlib import Path
import ctypes as C
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class Model(C.Structure):
    _fields_=[('volume',C.c_uint8),('running',C.c_bool),('volume_revision',C.c_uint32)]
class Command(C.Structure):
    _fields_=[('kind',C.c_int),('delta',C.c_int)]
class AudioControlTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory(); cls.addClassCleanup(cls.temp.cleanup)
        lib=Path(cls.temp.name)/'control.so'
        includes=['-Icomponents/'+n+'/include' for n in ('graphics','ui','ui_widgets','input','audio_gain')]
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',*includes,'-Imain',
            'main/audio_control_model.c','components/audio_gain/streamoo_audio_gain.c','-o',str(lib)],cwd=ROOT,check=True)
        cls.lib=C.CDLL(str(lib))
        cls.lib.streamoo_audio_control_model_init.argtypes=[C.POINTER(Model),C.c_uint8]
        cls.lib.streamoo_audio_control_model_init.restype=C.c_bool
        cls.lib.streamoo_audio_control_model_apply.argtypes=[C.POINTER(Model),C.POINTER(Command)]
        cls.lib.streamoo_audio_control_model_apply.restype=C.c_bool
        cls.lib.streamoo_audio_control_model_gain.argtypes=[C.POINTER(Model),C.POINTER(C.c_uint32)]
        cls.lib.streamoo_audio_control_model_gain.restype=C.c_bool
    def apply(self,m,kind,delta=0):
        return self.lib.streamoo_audio_control_model_apply(C.byref(m),C.byref(Command(kind,delta)))
    def gain(self,m):
        out=C.c_uint32(); self.assertTrue(self.lib.streamoo_audio_control_model_gain(C.byref(m),C.byref(out)))
        return out.value
    def test_restored_volume_starts_stopped(self):
        m=Model(); self.assertTrue(self.lib.streamoo_audio_control_model_init(C.byref(m),37))
        self.assertEqual(m.volume,37); self.assertFalse(m.running); self.assertEqual(self.gain(m),0)
    def test_volume_clamp_and_revision(self):
        m=Model(50,False,0)
        self.assertTrue(self.apply(m,0,2147483647)); self.assertEqual(m.volume,100)
        self.assertEqual(m.volume_revision,1)
        self.apply(m,0,1); self.assertEqual(m.volume_revision,1)
        self.apply(m,0,-2147483648); self.assertEqual(m.volume,0)
        self.assertEqual(m.volume_revision,2); self.assertEqual(self.gain(m),0)
    def test_stop_keeps_volume_and_play_uses_gain(self):
        m=Model(100,False,0); self.apply(m,1)
        self.assertTrue(m.running); self.assertEqual(self.gain(m),65536)
        self.apply(m,1); self.assertFalse(m.running)
        self.assertEqual(m.volume,100); self.assertEqual(m.volume_revision,0)
        self.apply(m,0,-50); self.assertEqual(self.gain(m),0)
        self.apply(m,1); self.assertEqual(self.gain(m),2072)
    def test_coalesced_changes_are_observable(self):
        m=Model(50,False,0); self.apply(m,0,1); self.apply(m,0,-1)
        self.assertEqual(m.volume,50); self.assertEqual(m.volume_revision,2)
    def test_invalid_command_is_atomic(self):
        m=Model(50,True,5); before=bytes(m)
        self.assertFalse(self.apply(m,99)); self.assertEqual(bytes(m),before)
        self.assertFalse(self.lib.streamoo_audio_control_model_init(C.byref(m),101))
        self.assertEqual(bytes(m),before)
