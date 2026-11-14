from pathlib import Path
import csv
import importlib.util
import json
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('flac_fixture', ROOT / 'tools/generate_flac_network_fixture.py')
fixture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixture)


class FlacNetworkFixtureTests(unittest.TestCase):
    def test_reject_bad_urls_and_duration_before_creating_directory(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp) / 'new'
            for url in ('https://host:8000', 'http://user:key@host:8000', 'http://host:8000/file',
                        'http://host:8000?a=1', 'http://host:8000#tag', 'http://host:8000?',
                        'http://host:8000#', 'http://host:0',
                        'http://host:99999', 'http://host:bad', 'http://ho st:8000'):
                with self.subTest(url=url), self.assertRaises(ValueError):
                    fixture.generate(directory, url, 2)
                self.assertFalse(directory.exists())
            for seconds in (0, 1801):
                with self.assertRaises(ValueError):
                    fixture.generate(directory, 'http://host:8000', seconds)
                self.assertFalse(directory.exists())

    def test_existing_directory_and_file_are_retained(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            (directory / 'lan24.flac').write_bytes(b'keep original')
            with self.assertRaises(FileExistsError):
                fixture.generate(directory, 'http://host:8000', 2)
            self.assertEqual((directory / 'lan24.flac').read_bytes(), b'keep original')

    @unittest.skipUnless(shutil.which('ffmpeg') and shutil.which('ffprobe'), 'Requires host FFmpeg/ffprobe')
    def test_actual_signals_format_playlist_hash_and_reproducible_pcm(self):
        with tempfile.TemporaryDirectory() as temp:
            left, right = Path(temp) / 'left', Path(temp) / 'right'
            first = fixture.generate(left, 'http://192.168.1.20:8000/', 2)
            second = fixture.generate(right, 'http://192.168.1.20:8000', 2)
            self.assertEqual(first, second)
            self.assertEqual(first, json.loads((left / 'manifest.json').read_text()))
            with (left / 'playlist.csv').open(newline='') as source:
                rows = list(csv.reader(source, delimiter='\t'))
            self.assertEqual([row[1:] for row in rows], [
                ['http://192.168.1.20:8000/lan16.flac', '0'],
                ['http://192.168.1.20:8000/lan24.flac', '0'],
                ['http://192.168.1.20:8000/lan24-live.flac', '0']])
            decoded = {}
            for entry in first['files']:
                path = left / entry['file']
                self.assertEqual(entry['sha256'], fixture.file_hash(path))
                pcm = subprocess.check_output(['ffmpeg', '-v', 'error', '-i', str(path),
                                               '-f', 's32le', '-c:a', 'pcm_s32le', '-'], timeout=20)
                self.assertEqual(len(pcm), 44100 * 2 * 2 * 4)
                self.assertGreater(len(set(pcm)), 1)
                decoded[entry['file']] = pcm
            self.assertEqual(decoded['lan24.flac'], decoded['lan24-live.flac'])
            self.assertGreater(first['files'][1]['file_average_kbps'], 1500)
            self.assertLess(first['files'][0]['file_average_kbps'], 500)
