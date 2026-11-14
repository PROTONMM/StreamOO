import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import probe_playlist as probe

PAYLOAD={'streams':[{'codec_name':'aac','profile':'LC','sample_rate':'48000','channels':2}],
         'packets':[{'size':'400','duration_time':'0.025'}]*4,'format':{'format_name':'aac'}}
class PlaylistProbeTests(unittest.TestCase):
 def test_packet_bitrate_codec_profile(self):
  r=probe.describe(PAYLOAD)
  self.assertEqual((r['status'],r['codec'],r['profile'],r['bitrate_bps']),('ACTIVE','aac','LC',128000))
  self.assertEqual(r['bitrate_source'],'packet_sample')
 def test_fallback_unknown_and_no_audio(self):
  payload={'streams':[{'codec_name':'mp3','bit_rate':'96000'}],'packets':[{'size':'3'}]}
  self.assertEqual(probe.describe(payload)['bitrate_bps'],96000)
  del payload['streams'][0]['bit_rate'];self.assertIsNone(probe.describe(payload)['bitrate_bps'])
  self.assertEqual(probe.describe({})['status'],'NO_AUDIO')
  self.assertEqual(probe.describe({'streams':[{'codec_name':'aac'}]})['status'],'NO_AUDIO')
 def test_timeout_http_and_invalid_json(self):
  with patch.object(probe.subprocess,'run',side_effect=subprocess.TimeoutExpired('ffprobe',1)):
   self.assertEqual(probe.probe_once('http://example.invalid','ffprobe',1)['status'],'TIMEOUT')
  for stderr,status in ((b'HTTP error 404 Not Found','HTTP_404'),(b'Server returned 410 Gone','HTTP_410'),(b'private-url-token','PROBE_ERROR')):
   with patch.object(probe.subprocess,'run',return_value=subprocess.CompletedProcess([],1,b'',stderr)):
    self.assertEqual(probe.probe_once('http://example.invalid','ffprobe',1),{'status':status})
  with patch.object(probe.subprocess,'run',return_value=subprocess.CompletedProcess([],0,b'bad json',b'')):
   self.assertEqual(probe.probe_once('http://example.invalid','ffprobe',1)['status'],'INVALID_RESPONSE')
 def test_retry_success_and_limits(self):
  with patch.object(probe,'probe_once',side_effect=[{'status':'TIMEOUT'},{'status':'ACTIVE'}]) as fn,patch.object(probe.time,'sleep'):
   self.assertEqual(probe.probe('http://example.invalid','ffprobe',1,2)['attempts'],2)
   self.assertEqual(fn.call_count,2)
 def test_normalization_literal_format_and_preserved_volume(self):
  with tempfile.TemporaryDirectory() as tmp:
   p=Path(tmp)/'in.csv';data='\ufeffZażółć; "test"\t\u200bhttp://example.invalid/a\t+12\r\n'
   p.write_text(data);rows=probe.load_playlist(p)
   self.assertEqual(rows[0]['url'],'http://example.invalid/a');self.assertTrue(rows[0]['normalized'])
   self.assertEqual(rows[0]['volume'],'+12');self.assertEqual(p.read_bytes(),data.encode())
   p.write_text('bad,data')
   with self.assertRaises(ValueError):probe.load_playlist(p)
 def test_clean_outputs_keep_duplicates_order_and_original(self):
  with tempfile.TemporaryDirectory() as tmp:
   base=Path(tmp);p=base/'in.csv';data='A\thttp://example.invalid/a\t1\nB\thttp://example.invalid/b\t2\nC\thttp://example.invalid/a\t3\n'
   p.write_text(data);rows=probe.load_playlist(p);results={'http://example.invalid/a':probe.describe(PAYLOAD),'http://example.invalid/b':{'status':'TIMEOUT'}}
   self.assertEqual(probe.write_outputs(rows,results,base/'out.csv',base/'report.json'),2)
   self.assertEqual((base/'out.csv').read_text(),data.splitlines(True)[0]+data.splitlines(True)[2])
   self.assertEqual(p.read_text(),data)
   with self.assertRaises(FileExistsError):probe.write_outputs(rows,results,p,base/'report2.json')
   self.assertEqual(p.read_text(),data)
   self.assertEqual(probe.write_outputs(rows,results,base/'keep.csv',base/'keep.json',True),3)
 def test_cli_with_fake_probe_and_duplicate_url_once(self):
  with tempfile.TemporaryDirectory() as tmp:
   base=Path(tmp);p=base/'playlist.csv';p.write_text('A\thttp://example.invalid/a\t0\nB\thttp://example.invalid/a\t1\n')
   exe=base/'ffprobe';count=base/'calls'
   exe.write_text('#!'+sys.executable+'\nimport json\nfrom pathlib import Path\nPath('+repr(str(count))+').open("a").write("x")\nprint('+repr(json.dumps(PAYLOAD))+')\n');exe.chmod(0o700)
   run=subprocess.run([sys.executable,str(ROOT/'tools/probe_playlist.py'),str(p),'--ffprobe',str(exe)],capture_output=True,text=True,check=True)
   self.assertIn('aac / LC',run.stdout);self.assertIn('128.0',run.stdout)
   self.assertEqual(count.read_text(),'x');self.assertEqual(len((base/'playlist.active.csv').read_text().splitlines()),2)
   again=subprocess.run([sys.executable,str(ROOT/'tools/probe_playlist.py'),str(p),'--ffprobe',str(exe)],capture_output=True)
   self.assertEqual(again.returncode,2);self.assertEqual(count.read_text(),'x')
