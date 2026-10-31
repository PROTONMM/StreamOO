from pathlib import Path
import hashlib
import importlib.util
import json
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('playlist_audit',ROOT/'tools/check_playlist.py')
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
class PlaylistAuditTests(unittest.TestCase):
    def row(self,name='Przykładowa stacja',url='https://example.invalid/stream',volume='0'):
        return f'{name}\t{url}\t{volume}'.encode()
    def codes(self,result):return {e['code'] for e in result['errors']}
    def test_utf8_three_literal_tab_fields(self):
        result=audit.audit(self.row(name='Zażółć, gęślą; "jaźń"'))
        self.assertEqual(result['valid_rows'],1);self.assertFalse(result['errors'])
    def test_bom_crlf_and_final_newline(self):
        result=audit.audit(b'\xef\xbb\xbf'+self.row()+b'\r\n'+self.row(url='http://example.invalid/live',volume='-5')+b'\r\n')
        self.assertEqual(result['valid_rows'],2);self.assertFalse(result['errors'])
    def test_invisible_prefix_is_not_silently_removed(self):
        result=audit.audit(self.row(url='\u200bhttps://example.invalid/stream'))
        self.assertIn('invisible_or_control_url_character',self.codes(result));self.assertEqual(result['invalid_rows'],1)
    def test_duplicates_are_reported_without_dropping_rows(self):
        result=audit.audit(self.row()+b'\n'+self.row(name='Inna nazwa'))
        self.assertEqual(result['valid_rows'],2);self.assertEqual(result['duplicates'],[{'line':2,'first_line':1}])
        self.assertFalse(result['errors'])
    def test_format_and_control_rejection(self):
        for data in (b'a,b,c',self.row()+b'\textra',self.row(name='bad\0name'),self.row(url='https://example.invalid/a b')):
            self.assertTrue(audit.audit(data)['errors'])
    def test_url_validation(self):
        for url in ('file:///secret','http://','https://example.invalid:99999/x','http://example.invalid:0/',
                    'https://user:secret@example.invalid/','https://example.invalid/x#fragment'):
            self.assertTrue(audit.audit(self.row(url=url))['errors'])
    def test_whole_integer_without_assumed_gain_scale(self):
        for value in ('0','63','-20','+10',str(2**31-1),str(-2**31)):
            self.assertFalse(audit.audit(self.row(volume=value))['errors'])
        for value in ('1junk','1.5','',str(2**31),str(-2**31-1)):
            self.assertTrue(audit.audit(self.row(volume=value))['errors'])
    def test_size_utf8_and_empty(self):
        for data in (b'',b'\xff',b'x'*(audit.MAX_BYTES+1),b'\n'*(audit.MAX_ROWS+1)):
            self.assertTrue(audit.audit(data)['errors'])
    def test_cli_is_read_only_and_redacts_content(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'playlist.csv'
            original=self.row(name='PRIVATE_STATION_NAME',url='\u200bhttps://private.example.invalid/key')
            path.write_bytes(original);before=hashlib.sha256(original).digest()
            result=subprocess.run(['python3',str(ROOT/'tools/check_playlist.py'),str(path)],capture_output=True,text=True)
            self.assertEqual(result.returncode,1);self.assertEqual(hashlib.sha256(path.read_bytes()).digest(),before)
            self.assertNotIn('PRIVATE_STATION_NAME',result.stdout+result.stderr)
            self.assertNotIn('private.example',result.stdout+result.stderr)
            self.assertEqual(json.loads(result.stdout)['invalid_rows'],1)
