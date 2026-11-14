"""Native configuration parsers; synthetic station data only."""
import ctypes as C
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
class Profile(C.Structure):
    _fields_ = [('ssid', C.c_char*33), ('password', C.c_char*64)]
class Profiles(C.Structure):
    _fields_ = [('count', C.c_uint8), ('profiles', Profile*8)]
class Seen(C.Structure):
    _fields_ = [('hash', C.c_uint64), ('offset', C.c_uint32), ('bytes', C.c_uint16)]
class Report(C.Structure):
    _fields_ = [('rows', C.c_uint32), ('normalized', C.c_uint32), ('duplicates', C.c_uint32), ('line', C.c_uint32), ('error', C.c_char_p)]
class ConfigurationModelTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(); cls.addClassCleanup(cls.tmp.cleanup)
        target = Path(cls.tmp.name)/'model.so'
        subprocess.run(['cc','-std=c11','-D_POSIX_C_SOURCE=200809L','-Wall','-Wextra','-Werror','-shared','-fPIC',
                        '-Icomponents/configuration/include','-Imanaged_components/espressif__cjson/cJSON',
                        'components/configuration/streamoo_config_model.c','managed_components/espressif__cjson/cJSON/cJSON.c',
                        '-lm','-o',str(target)], cwd=ROOT, check=True)
        cls.lib=C.CDLL(str(target))
        cls.parse=cls.lib.streamoo_wifi_profiles_parse
        cls.parse.argtypes=[C.c_char_p,C.c_size_t,C.POINTER(Profiles),C.POINTER(Profiles)];cls.parse.restype=C.c_bool
        cls.encode=cls.lib.streamoo_wifi_profiles_encode
        cls.encode.argtypes=[C.POINTER(Profiles),C.c_void_p,C.c_size_t];cls.encode.restype=C.c_size_t
        cls.decode=cls.lib.streamoo_wifi_profiles_decode
        cls.decode.argtypes=[C.c_void_p,C.c_size_t,C.POINTER(Profiles)];cls.decode.restype=C.c_bool
        cls.csv=cls.lib.streamoo_playlist_validate
        cls.csv.argtypes=[C.c_void_p,C.POINTER(C.c_size_t),C.POINTER(Report),C.c_void_p];cls.csv.restype=C.c_bool
    def parsed(self, data, old=None):
        raw=data if isinstance(data,bytes) else json.dumps(data,ensure_ascii=False).encode()
        out=Profiles();ok=self.parse(raw,len(raw),C.byref(old) if old else None,C.byref(out));return ok,out
    def test_roundtrip_retention_order_and_explicit_open(self):
        ok,p=self.parsed({'profiles':[{'ssid':'Żółw','password':'test-only-key'},{'ssid':'Open','password':''}]});self.assertTrue(ok)
        ok,q=self.parsed({'profiles':[{'ssid':'Open'},{'ssid':'Żółw'}]},p);self.assertTrue(ok)
        self.assertEqual(q.profiles[1].password,b'test-only-key')
        blob=C.create_string_buffer(1024);n=self.encode(C.byref(q),blob,len(blob));self.assertGreater(n,9)
        r=Profiles();self.assertTrue(self.decode(blob,n,C.byref(r)));self.assertEqual(bytes(q),bytes(r))
        blob[6]=b'!';self.assertFalse(self.decode(blob,n,C.byref(r)))
        ok,r=self.parsed({'profiles':[{'ssid':'Żółw','password':''}]},p);self.assertTrue(ok);self.assertEqual(r.profiles[0].password,b'')
    def test_invalid_profiles(self):
        cases=[{}, {'profiles':None},{'profiles':[{'ssid':'New'}]}, {'profiles':[{'ssid':'','password':''}]},
               {'profiles':[{'ssid':'a'*33,'password':''}]},{'profiles':[{'ssid':'a','password':'short'}]},
               {'profiles':[{'ssid':'a','password':''}]*2},{'profiles':[{'ssid':str(i),'password':''} for i in range(9)]},
               {'profiles':[{'ssid':'a','password':'x'*64}]},{'profiles':[{'ssid':'a\x00z','password':''}]},
               {'profiles':[{'ssid':'a','password':'x'*7+'\n'}]}, {'profiles':[], 'extra':0},
               b'{"profiles":[],"profiles":[]}', b'{"profiles":[{"ssid":"a","ssid":"b","password":""}]}',
               b'{"profiles":[]}x', b'{"profiles":[[[[[[]]]]]]}', b'{"profiles":[{"ssid":"\xff","password":""}]}']
        for case in cases:
            with self.subTest(case=case):self.assertFalse(self.parsed(case)[0])
    def test_limits_empty_and_truncated_persistence(self):
        self.assertTrue(self.parsed({'profiles':[]})[0])
        ok,p=self.parsed({'profiles':[{'ssid':str(i)+'a'*31,'password':'p'*63} for i in range(8)]});self.assertTrue(ok)
        blob=C.create_string_buffer(1024);n=self.encode(C.byref(p),blob,1024);out=Profiles()
        for length in range(n):self.assertFalse(self.decode(blob,length,C.byref(out)))
        self.assertEqual(self.encode(C.byref(p),blob,n-1),0)
    def validate(self, raw):
        buf=C.create_string_buffer(raw);n=C.c_size_t(len(raw));r=Report();workspace=(Seen*2048)()
        ok=self.csv(buf,C.byref(n),C.byref(r),workspace);return ok,r,buf.raw[:n.value]
    def test_csv_normalization_duplicates_and_integer_limits(self):
        raw='\ufeffŻółw\thttp://radio.invalid/live\t-2147483648\r\nInny\t\u200bhttp://radio.invalid/live\t2147483647'.encode()
        ok,r,out=self.validate(raw);self.assertTrue(ok);self.assertEqual((r.rows,r.normalized,r.duplicates,r.line),(2,1,1,0))
        self.assertNotIn(b'\r',out);self.assertNotIn(b'\xef\xbb\xbf',out);self.assertNotIn(b'\xe2\x80\x8b',out)
        self.assertTrue(self.validate(out)[0]);self.assertEqual(self.validate(out)[2],out)
    def test_csv_errors_report_line_without_echoing_content(self):
        for row in [b'',b'a,b,c',b'a\tftp://radio.invalid\t0',b'a\thttp://user:pass@radio.invalid\t0',
                    b'a\thttp://radio.invalid:65536\t0',b'a\thttp://radio.invalid\t2147483648',
                    b'a\thttp://radio.invalid\t-',b'\xff\thttp://radio.invalid\t0',b'a\thttp://radio.invalid\t0\textra']:
            ok,r,_=self.validate(b'OK\thttp://ok.invalid\t0\n'+row+b'\n');self.assertFalse(ok);self.assertEqual(r.line,2)
            self.assertNotIn(b'radio.invalid',r.error)
    def test_csv_size_and_rows_bounds(self):
        for raw in [b'',b'\xef\xbb\xbf',b'x'*(256*1024+1),b'a\thttp://a.invalid\t0\n'*2049]:self.assertFalse(self.validate(raw)[0])
