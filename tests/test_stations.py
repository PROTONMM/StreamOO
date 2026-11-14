"""Catalog identity and immutable-string projection, synthetic stations only."""
import ctypes as C
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class Station(C.Structure):
    _fields_=[('id',C.c_uint64),('name',C.c_char_p),('url',C.c_char_p),('name_bytes',C.c_size_t),('url_bytes',C.c_size_t),('ovol',C.c_int32)]
class StationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup)
        lib=Path(cls.tmp.name)/'stations.so'
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC','-Icomponents/stations/include','components/stations/streamoo_stations.c','-o',str(lib)],cwd=ROOT,check=True)
        cls.lib=C.CDLL(str(lib));cls.build=cls.lib.streamoo_stations_build
        cls.build.argtypes=[C.c_void_p,C.c_size_t,C.POINTER(Station),C.c_size_t,C.POINTER(C.c_size_t)];cls.build.restype=C.c_bool
    def parse(self,raw,capacity=2048):
        buf=C.create_string_buffer(raw);entries=(Station*capacity)();count=C.c_size_t()
        ok=self.build(buf,len(raw),entries,capacity,C.byref(count))
        return ok,[(s.id,s.name,s.url,s.ovol) for s in entries[:count.value]]
    def test_reorder_and_ovol_change_preserve_identity(self):
        a='Łódź\thttp://a.invalid/live\t0';b='Druga\thttps://b.invalid/live\t-2147483648'
        ok,first=self.parse((a+'\n'+b).encode());self.assertTrue(ok)
        ok,second=self.parse((b+'\n'+a[:-1]+'63\n').encode());self.assertTrue(ok)
        self.assertEqual(first[0][0],second[1][0]);self.assertEqual(first[1][0],second[0][0]);self.assertEqual(second[1][3],63)
        self.assertEqual(first[0][1],'Łódź'.encode());self.assertEqual(first[1][3],-2147483648)
    def test_duplicate_urls_and_identical_rows_remain_distinct(self):
        a=b'A\thttp://same.invalid\t0';b=b'B\thttp://same.invalid\t0'
        ok,rows=self.parse(a+b'\n'+a+b'\n'+b);self.assertTrue(ok)
        self.assertEqual(len({s[0] for s in rows}),3)
        self.assertNotIn(0,{s[0] for s in rows})
        self.assertEqual(rows,self.parse(a+b'\n'+a+b'\n'+b)[1])
        reordered=self.parse(b+b'\n'+a+b'\n'+a)[1]
        self.assertEqual({r[0] for r in rows},{r[0] for r in reordered})
    def test_invalid_shapes_capacity_and_integer_range(self):
        for data in (b'',b'bad',b'\thttp://a.invalid\t0',b'A\t\t0',b'A\thttp://a.invalid\t',b'A\thttp://a.invalid\t2147483648',b'A\thttp://a.invalid\t-2147483649',b'A\thttp://a.invalid\t0\tx',b'A\x00B\thttp://a.invalid\t0'):
            self.assertFalse(self.parse(data)[0])
        self.assertFalse(self.parse(b'A\thttp://a.invalid\t0\nB\thttp://b.invalid\t0',1)[0])
