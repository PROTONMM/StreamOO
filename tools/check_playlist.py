#!/usr/bin/env python3
"""Read-only audit of yoRadio-style TAB playlist.csv; never prints station data."""
import argparse
import json
from pathlib import Path
import re
import unicodedata
from urllib.parse import urlsplit

MAX_BYTES = 256 * 1024
MAX_ROWS = 2048

def audit(data):
    result = {'rows': 0, 'valid_rows': 0, 'invalid_rows': 0, 'duplicates': [], 'errors': [], 'warnings': []}
    def error(line, code):
        result['errors'].append({'line': line, 'code': code})
    if len(data) > MAX_BYTES:
        error(0, 'file_too_large')
        return result
    try:
        text = data.decode('utf-8-sig')
    except UnicodeDecodeError:
        error(0, 'invalid_utf8')
        return result
    lines = text.split('\n')
    if lines and lines[-1] == '':
        lines.pop()
    if not lines or lines == ['']:
        error(0, 'empty_file')
        return result
    if len(lines) > MAX_ROWS:
        error(0, 'too_many_rows')
        return result
    seen = {}
    for number, raw in enumerate(lines, 1):
        result['rows'] += 1
        if raw.endswith('\r'):
            raw = raw[:-1]
        fields = raw.split('\t')  # Literal TAB fields, no RFC4180 quoting.
        before = len(result['errors'])
        if len(fields) != 3:
            error(number, 'expected_three_tab_fields')
        else:
            name, url, volume = fields
            if not name or len(name.encode()) > 128:
                error(number, 'name_length')
            if any(unicodedata.category(c) in ('Cc', 'Cf') for c in name):
                error(number, 'name_control_character')
            if not url or len(url.encode()) > 2048:
                error(number, 'url_length')
            elif any(unicodedata.category(c) in ('Cc', 'Cf') for c in url):
                error(number, 'invisible_or_control_url_character')
            elif any(c.isspace() for c in url):
                error(number, 'url_whitespace')
            else:
                try:
                    parsed = urlsplit(url)
                    port = parsed.port
                    if parsed.scheme not in ('http', 'https') or not parsed.hostname or port == 0:
                        error(number, 'invalid_http_url')
                    if parsed.username is not None or parsed.password is not None:
                        error(number, 'url_credentials')
                    if parsed.fragment:
                        error(number, 'url_fragment')
                except ValueError:
                    error(number, 'invalid_http_url')
            if not re.fullmatch(r'[+-]?[0-9]+', volume) or len(volume) > 11:
                error(number, 'invalid_ovol_integer')
            elif not -(2**31) <= int(volume) < 2**31:
                error(number, 'ovol_out_of_range')
            if url in seen:
                duplicate = {'line': number, 'first_line': seen[url]}
                result['duplicates'].append(duplicate)
                result['warnings'].append({'line': number, 'code': 'duplicate_url'})
            else:
                seen[url] = number
        if len(result['errors']) == before:
            result['valid_rows'] += 1
        else:
            result['invalid_rows'] += 1
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('file', type=Path)
    args = parser.parse_args()
    try:
        with args.file.open('rb') as source:
            report = audit(source.read(MAX_BYTES + 1))
    except OSError:
        parser.exit(2, 'Cannot read playlist; values omitted.\n')
    print(json.dumps(report, indent=2, ensure_ascii=True))
    return 1 if report['errors'] else 0

if __name__ == '__main__':
    raise SystemExit(main())
