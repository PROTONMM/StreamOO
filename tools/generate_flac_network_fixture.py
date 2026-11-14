#!/usr/bin/env python3
"""Create long, original FLAC files and a private LAN test playlist.

FFmpeg runs on the computer only. The firmware and public station stay unchanged.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
from urllib.parse import urlsplit


def base_url(value):
    parsed = urlsplit(value)
    try:
        port = parsed.port
    except ValueError as error:
        raise ValueError('Invalid HTTP port') from error
    if (parsed.scheme != 'http' or not parsed.hostname or parsed.username is not None
            or parsed.password is not None or any(c in value for c in '?#')
            or parsed.path not in ('', '/') or (port is not None and not 1 <= port <= 65535)
            or any(c.isspace() for c in value)):
        raise ValueError('Use an HTTP base URL without credentials, path, query or fragment')
    return value.rstrip('/')


def file_hash(path):
    digest = hashlib.sha256()
    with path.open('rb') as source:
        for block in iter(lambda: source.read(65536), b''):
            digest.update(block)
    return digest.hexdigest()


def generate(directory, address, seconds):
    address = base_url(address)
    if not 1 <= seconds <= 1800:
        raise ValueError('Duration must be 1..1800 seconds')
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=False)
    variants = [
        ('TEST LAN FLAC16 tones', 'lan16.flac', 16,
         [f'sine=frequency=997:sample_rate=44100:duration={seconds}',
          f'sine=frequency=2003:sample_rate=44100:duration={seconds}']),
        ('TEST LAN FLAC24 noise', 'lan24.flac', 24,
         [f'anoisesrc=sample_rate=44100:duration={seconds}:amplitude=0.08:seed={seed}'
          for seed in (1001, 2002)]),
    ]
    manifest = {'source': 'Original mathematical signals; FFmpeg on host only',
                'sample_rate': 44100, 'seconds': seconds, 'files': []}
    for name, filename, bits, sources in variants:
        path = directory / filename
        command = ['ffmpeg', '-nostdin', '-hide_banner', '-loglevel', 'error', '-n']
        for source in sources:
            command += ['-f', 'lavfi', '-i', source]
        command += ['-filter_complex', '[0:a][1:a]amerge=inputs=2[a]', '-map', '[a]',
                    '-map_metadata', '-1', '-c:a', 'flac', '-compression_level', '5',
                    '-sample_fmt', 's16' if bits == 16 else 's32',
                    '-bits_per_raw_sample', str(bits), str(path)]
        subprocess.run(command, check=True, timeout=max(60, seconds))
        probe = json.loads(subprocess.check_output(
            ['ffprobe', '-v', 'error', '-show_streams', '-show_format', '-of', 'json', str(path)],
            timeout=30))
        stream = probe['streams'][0]
        duration = float(probe['format']['duration'])
        if (stream['codec_name'] != 'flac' or int(stream['sample_rate']) != 44100
                or stream['channels'] != 2 or int(stream['bits_per_raw_sample']) != bits
                or abs(duration - seconds) > 1 / 44100):
            raise ValueError(f'Unexpected generated format: {filename}')
        manifest['files'].append({'name': name, 'file': filename, 'bits': bits,
                                  'bytes': path.stat().st_size, 'sha256': file_hash(path),
                                  'file_average_kbps': round(path.stat().st_size * 8 / duration / 1000, 2)})
    # Same encoded frames, without file metadata: exercise live STREAMINFO
    # bootstrap independently of Internet/TLS/ICY. No decoding/re-encoding.
    live = directory / 'lan24-live.flac'
    subprocess.run(['ffmpeg', '-nostdin', '-hide_banner', '-loglevel', 'error', '-n',
                    '-i', str(directory / 'lan24.flac'), '-map', '0:a:0', '-c:a', 'copy',
                    '-write_header', '0', str(live)], check=True, timeout=max(60, seconds))
    with live.open('rb') as source:
        prefix = source.read(2)
    if len(prefix) != 2 or prefix[0] != 255 or prefix[1] & 0xFE != 0xF8:
        raise ValueError('Headerless fixture must begin with a native FLAC frame')
    manifest['files'].append({'name': 'TEST LAN FLAC24 live headerless', 'file': live.name,
                              'bits': 24, 'bytes': live.stat().st_size, 'sha256': file_hash(live),
                              'file_average_kbps': round(live.stat().st_size * 8 / seconds / 1000, 2)})
    with (directory / 'playlist.csv').open('w', newline='', encoding='utf-8') as output:
        writer = csv.writer(output, delimiter='\t', lineterminator='\n')
        for entry in manifest['files']:
            writer.writerow([entry['name'], address + '/' + entry['file'], 0])
    (directory / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, required=True, help='New directory; existing files are never overwritten')
    parser.add_argument('--base-url', required=True, help='HTTP address of this computer visible to the radio, e.g. http://192.168.1.20:8000')
    parser.add_argument('--seconds', type=int, default=300, help='Duration of each file: 1..1800s, default 300')
    args = parser.parse_args()
    try:
        manifest = generate(args.output_dir, args.base_url, args.seconds)
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        parser.exit(1, f'Fixture generation failed: {error}\n')
    for entry in manifest['files']:
        print(f"{entry['name']}: {entry['file_average_kbps']} kbps file average; SHA256={entry['sha256']}")
    port = urlsplit(args.base_url).port or 80
    print('Serve with: ' + shlex.join(['python3', '-m', 'http.server', str(port),
                                     '--bind', '0.0.0.0', '--directory', str(args.output_dir)]))
    print(f'Import {args.output_dir / "playlist.csv"}; play each station separately.')
    print(f'Each file ends after {args.seconds}s. EOF/retry at that point is expected.')
    print('HTTP LAN excludes Internet/TLS/ICY, but still uses Wi-Fi, the HTTP ring, decoder, PCM and OLED.')


if __name__ == '__main__':
    main()
