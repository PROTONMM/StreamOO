"""Protect retained settings and verify the no-OTA 16MiB layout."""
import csv
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
class PartitionLayoutTests(unittest.TestCase):
 def test_single_large_app_and_separate_station_store(self):
  lines=[line for line in (ROOT/'partitions.csv').read_text().splitlines() if line and not line.startswith('#')]
  rows=[tuple(value.strip() for value in row) for row in csv.reader(lines)]
  by_name={row[0]:row for row in rows}
  apps=[row for row in rows if row[1]=='app']
  self.assertEqual(len(apps),1)
  self.assertEqual(apps[0][2],'factory')
  self.assertGreater(int(apps[0][4],0),14*1024*1024)
  self.assertEqual(int(apps[0][3],0),0x10000)
  self.assertEqual((int(by_name['nvs'][3],0),int(by_name['nvs'][4],0)),(0x9000,0x6000))
  self.assertEqual((int(by_name['phy_init'][3],0),int(by_name['phy_init'][4],0)),(0xf000,0x1000))
  self.assertEqual(by_name['storage'][1:3],('data','littlefs'))
  self.assertEqual(int(by_name['storage'][4],0),1024*1024)
  self.assertFalse(any(row[2]=='ota' or row[2].startswith('ota_') for row in rows))
  end=0x9000
  for row in sorted(rows,key=lambda r:int(r[3],0)):
   start,size=int(row[3],0),int(row[4],0)
   self.assertGreaterEqual(start,end)
   self.assertEqual(start%0x1000,0)
   self.assertEqual(size%0x1000,0)
   end=start+size
   self.assertLessEqual(end,16*1024*1024)
  self.assertEqual(end,16*1024*1024)
