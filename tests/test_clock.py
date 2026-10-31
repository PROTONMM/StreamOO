import ctypes as C
from datetime import datetime,timedelta,timezone
from pathlib import Path
import subprocess,tempfile,unittest
from zoneinfo import ZoneInfo
ROOT=Path(__file__).resolve().parents[1]
class Civil(C.Structure):
 _fields_=[('year',C.c_uint16)]+[(n,C.c_uint8) for n in ('month','day','hour','minute','second')]
class ClockTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup)
  lib=str(Path(cls.tmp.name)/'clock.so')
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC','-Icomponents/clock/include','components/clock/streamoo_clock.c','-o',lib],cwd=ROOT,check=True)
  cls.lib=C.CDLL(lib);cls.project=cls.lib.streamoo_clock_project
  cls.project.argtypes=[C.c_int64,C.c_bool,C.c_int,C.POINTER(Civil)];cls.project.restype=C.c_bool
  cls.name=cls.lib.streamoo_clock_zone_name;cls.name.argtypes=[C.c_int];cls.name.restype=C.c_char_p
  cls.parse=cls.lib.streamoo_clock_zone_parse;cls.parse.argtypes=[C.c_char_p,C.POINTER(C.c_int)];cls.parse.restype=C.c_bool
 def check(self,utc,zone):
  out=Civil();self.assertTrue(self.project(int(utc.timestamp()),True,zone,C.byref(out)))
  expected=utc.astimezone(ZoneInfo(self.name(zone).decode()))
  self.assertEqual(tuple(getattr(out,n) for n,_ in Civil._fields_),tuple(getattr(expected,n) for n,_ in Civil._fields_))
 def test_unsynchronized_invalid_and_bounds_clear(self):
  for epoch,synced,zone in ((1793448000,False,1),(0,True,1),(4102444800,True,1),(1793448000,True,99)):
   out=Civil(2026,1,1,1,1,1);self.assertFalse(self.project(epoch,synced,zone,C.byref(out)))
   self.assertEqual(out.year,0);self.assertEqual(out.second,0)
  self.assertFalse(self.project(1793448000,True,1,None))
 def test_zone_names_roundtrip(self):
  for i in range(8):
   value=C.c_int();self.assertTrue(self.parse(self.name(i),C.byref(value)));self.assertEqual(value.value,i)
  self.assertIsNone(self.name(99));self.assertFalse(self.parse(b'Unknown/Zone',C.byref(C.c_int())))
 def test_weekly_dates_2020_through_2099_all_zones(self):
  utc=datetime(2020,1,1,23,37,29,tzinfo=timezone.utc)
  while utc.year<2100:
   for zone in range(8):self.check(utc,zone)
   utc+=timedelta(days=7)
 def test_dst_exact_transitions_every_year(self):
  for year in range(2020,2100):
   for month in (3,10):
    day=datetime(year,month,31,1,tzinfo=timezone.utc)
    day-=timedelta(days=(day.weekday()+1)%7)
    for offset in (-1,0,1):
     for zone in range(8):self.check(day+timedelta(seconds=offset),zone)
 def test_leap_day_midnight_and_year_boundary(self):
  for value in ('2024-02-28T23:59:59','2024-02-29T23:59:59','2026-12-31T23:59:59','2099-12-31T23:59:59'):
   for zone in range(8):self.check(datetime.fromisoformat(value).replace(tzinfo=timezone.utc),zone)
