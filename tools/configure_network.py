#!/usr/bin/env python3
"""Private AP build configuration. Never accept a password as a command argument."""
import argparse
import getpass
import json
import os
from pathlib import Path
import shlex
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def default_file():
    return Path(os.environ.get('XDG_CONFIG_HOME') or Path.home()/'.config')/'streamoo/network.json'

def validate_password(value):
    return isinstance(value, str) and 8 <= len(value) <= 63 and all(32 <= ord(c) <= 126 for c in value)

def external_path(path):
    path = Path(path).expanduser().resolve()
    if path == ROOT or ROOT in path.parents:
        raise ValueError('Secret file must be outside the repository.')
    return path

def load_password(path):
    value = json.loads(external_path(path).read_text(encoding='utf-8'))
    if not isinstance(value, dict) or value.get('version') != 1 or not validate_password(value.get('ap_password')):
        raise ValueError('Invalid private AP configuration.')
    return value['ap_password']

def generate_header(path, output, explicit=False):
    path = external_path(path)
    if not path.exists() and not explicit:
        text = '#pragma once\n#define STREAMOO_NETWORK_SECRET_AVAILABLE 0\n'
    else:
        password = load_password(path)
        numbers = ','.join(str(b) for b in password.encode('ascii') + b'\0')
        text = ('#pragma once\n#define STREAMOO_NETWORK_SECRET_AVAILABLE 1\n'
                'static const unsigned char streamoo_ap_password[] = {' + numbers + '};\n')
    output = Path(output).resolve()
    if ROOT in output.parents and ROOT/'build' not in output.parents:
        raise ValueError('Generated secret header must be in ignored build/ or outside repository.')
    output.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix='.network-secret-', dir=output.parent)
    try:
        with os.fdopen(fd, 'w', encoding='ascii') as out:
            out.write(text)
        os.replace(temporary, output)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)

def save_private(path, password):
    path = external_path(path)
    if not validate_password(password):
        raise ValueError('AP password requires 8..63 printable ASCII characters.')
    path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
    fd, temporary = tempfile.mkstemp(prefix='.network-', dir=path.parent)
    try:
        with os.fdopen(fd, 'w', encoding='utf-8') as out:
            json.dump({'version': 1, 'ap_password': password}, out)
            out.write('\n')
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--file', type=Path)
    parser.add_argument('--generate-header', type=Path, metavar='OUTPUT')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    path = args.file or default_file()
    try:
        if args.generate_header:
            generate_header(path, args.generate_header, args.file is not None)
        elif args.check:
            load_password(path)
            print('Private AP configuration valid; values omitted.')
        else:
            path = external_path(path)
            password = getpass.getpass('Wspólne hasło AP (8..63 znaków ASCII): ')
            confirmation = getpass.getpass('Powtórz hasło AP: ')
            if password != confirmation:
                raise ValueError('Password confirmation mismatch.')
            save_private(path, password)
            print('Private AP configuration saved outside repository; password omitted.')
            if args.file:
                print('idf.py -DSTREAMOO_NETWORK_SECRETS_FILE=' + shlex.quote(str(path)) + ' reconfigure build')
            else:
                print('idf.py reconfigure build')
    except (OSError, ValueError, TypeError, UnicodeError):
        # Parsing/I/O exceptions can contain source text; never echo them.
        parser.exit(1, 'Private AP configuration failed. Use an external JSON v1 file and an 8..63 printable ASCII password. Values omitted.\n')

if __name__ == '__main__':
    main()
