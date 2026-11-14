from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class InputTests(unittest.TestCase):
    def test_decoder_sequences_and_gestures(self):
        with tempfile.TemporaryDirectory() as directory:
            binary = str(Path(directory) / 'input')
            subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                            '-Icomponents/input/include', 'components/input/streamoo_input.c',
                            'tests/input_test.c', '-o', binary], cwd=ROOT, check=True)
            subprocess.run([binary], check=True)
