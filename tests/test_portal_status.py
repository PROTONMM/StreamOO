from pathlib import Path
import ctypes as C
import json
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class Status(C.Structure):
    _fields_=[('ssid',C.c_char*16),('clients',C.c_uint8),('volume',C.c_uint8),
              ('ready',C.c_bool),('available',C.c_bool),('running',C.c_bool),('buffering',C.c_bool),('uptime',C.c_uint64),('ip',C.c_char*16),('connected',C.c_bool),('configuring',C.c_bool),('profiles',C.c_uint8),('error',C.c_uint32)]
class PortalStatusTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.temp.cleanup)
        lib=Path(cls.temp.name)/'status.so'
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',
            '-Icomponents/portal/include','components/portal/streamoo_portal_status.c','-o',str(lib)],cwd=ROOT,check=True)
        cls.lib=C.CDLL(str(lib));cls.render=cls.lib.streamoo_portal_status_json
        cls.render.argtypes=[C.POINTER(Status),C.c_void_p,C.c_size_t];cls.render.restype=C.c_bool
    def state(self): return Status(b'StreamOO-123ABC',1,87,True,True,True,False,12345)
    def test_status_schema_and_real_values(self):
        out=C.create_string_buffer(512);self.assertTrue(self.render(C.byref(self.state()),out,512))
        value=json.loads(out.value)
        self.assertEqual(value['schema_version'],1)
        self.assertEqual(value['network'],{'mode':'ap','ready':True,'ssid':'StreamOO-123ABC','clients':1,'ap_ready':True,'connected':False,'configuring':False,'ip':'192.168.4.1','profiles':0,'error':0})
        self.assertEqual(value['audio'],{'available':True,'source':'test_fixture','running':True,'buffering':False,'volume':87})
        self.assertEqual(value['uptime_ms'],12345)
    def test_nonterminated_and_injectable_ssid_rejected(self):
        for ssid in (b'StreamOO-123ABCX',b'StreamOO-123abC',b'\"/><script>',b'OtherSSID'):
            s=self.state();s.ssid=ssid;out=C.create_string_buffer(512)
            self.assertFalse(self.render(C.byref(s),out,512))
    def test_empty_ssid_only_when_not_ready(self):
        s=Status();out=C.create_string_buffer(512)
        self.assertTrue(self.render(C.byref(s),out,512))
        s.ready=True;self.assertFalse(self.render(C.byref(s),out,512))
    def test_out_of_range_snapshot_rejected(self):
        out=C.create_string_buffer(512)
        for clients,volume in ((5,50),(0,101),(255,255)):
            s=self.state();s.clients=clients;s.volume=volume
            self.assertFalse(self.render(C.byref(s),out,512))
    def test_truncation_and_canaries(self):
        out=(C.c_ubyte*32)(*([77]*32));s=self.state()
        self.assertFalse(self.render(C.byref(s),C.cast(C.byref(out,1),C.c_void_p),30))
        self.assertEqual(out[0],77);self.assertEqual(out[31],77)
    def test_stop_buffering_and_max_uptime(self):
        s=self.state();s.running=False;s.buffering=True;s.uptime=2**64-1
        out=C.create_string_buffer(512);self.assertTrue(self.render(C.byref(s),out,512))
        value=json.loads(out.value);self.assertFalse(value['audio']['running'])
        self.assertTrue(value['audio']['buffering']);self.assertEqual(value['uptime_ms'],2**64-1)
    def test_sta_and_transition_addresses(self):
        s=self.state();s.connected=True;s.ip=b'192.168.1.73';s.profiles=3
        out=C.create_string_buffer(512)
        self.assertTrue(self.render(C.byref(s),out,512));value=json.loads(out.value)
        self.assertEqual(value['network']['mode'],'apsta');self.assertEqual(value['network']['ip'],'192.168.1.73')
        s.ready=False;self.assertTrue(self.render(C.byref(s),out,512));value=json.loads(out.value)
        self.assertEqual(value['network']['mode'],'sta');self.assertTrue(value['network']['ready'])
        s.ip=b'\"/><script>';self.assertFalse(self.render(C.byref(s),out,512))
