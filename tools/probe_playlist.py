#!/usr/bin/env python3
"""Sprawdź playlistę TAB i zapisz osobno aktywne stacje oraz raport kodeków."""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import json
import math
from pathlib import Path
import re
import shutil
import subprocess
import time

from check_playlist import audit, MAX_BYTES


def load_playlist(path):
    with path.open("rb") as stream:
        data = stream.read(MAX_BYTES+1)
    if len(data) > MAX_BYTES:
        raise ValueError('Playlista przekracza 256 KiB.')
    text = data.decode('utf-8-sig')
    rows = []
    lines=text.split("\n")
    if lines and not lines[-1]: lines.pop()
    for line, raw in enumerate(lines, 1):
        raw=raw.removesuffix("\r")
        fields = raw.split('\t')
        if len(fields) != 3:
            raise ValueError(f'Wiersz {line}: oczekiwano 3 pól TAB.')
        name, url, volume = fields
        normalized = url.startswith('\u200b')
        url = url.lstrip('\u200b')
        result = audit(f'{name}\t{url}\t{volume}'.encode())
        if result['errors']:
            raise ValueError(f'Wiersz {line}: {result["errors"][0]["code"]}.')
        rows.append(dict(line=line, name=name, url=url, volume=volume,
                         normalized=normalized))
    if not rows or len(rows) > 2048:
        raise ValueError('Wymagane 1..2048 stacji.')
    return rows


def describe(payload):
    streams = payload.get('streams') or []
    if not streams or not streams[0].get('codec_name') or not payload.get('packets'):
        return dict(status='NO_AUDIO')
    stream = streams[0]
    sizes, duration, packets = 0, 0.0, 0
    for packet in payload.get('packets', []):
        try:
            size, seconds = int(packet['size']), float(packet['duration_time'])
            if size > 0 and math.isfinite(seconds) and seconds > 0:
                sizes += size
                duration += seconds
                packets += 1
        except (KeyError, ValueError, TypeError):
            pass
    bitrate, source = None, 'unknown'
    if packets >= 2 and duration > 0:
        bitrate, source = round(sizes * 8 / duration), 'packet_sample'
    else:
        try:
            value = int(stream.get('bit_rate', 0))
            if value > 0:
                bitrate, source = value, 'ffprobe_stream'
        except (ValueError, TypeError):
            pass
    return dict(status='ACTIVE', codec=stream['codec_name'],
                profile=stream.get('profile', 'unknown'), bitrate_bps=bitrate,
                bitrate_source=source, sample_seconds=round(duration, 3),
                sample_rate=stream.get('sample_rate'), channels=stream.get('channels'),
                container=(payload.get('format') or {}).get('format_name', 'unknown'))


def probe_once(url, ffprobe, timeout):
    command = [ffprobe, '-v', 'error', '-rw_timeout', str(int(timeout*1_000_000)),
               '-tls_verify', '1', '-protocol_whitelist', 'http,https,tcp,tls,crypto',
               '-analyzeduration', '2000000', '-probesize', '524288',
               '-read_intervals', '%+#200', '-select_streams', 'a:0',
               '-show_entries', 'stream=codec_name,profile,sample_rate,channels,bit_rate:'
               'packet=size,duration_time:format=format_name', '-of', 'json', '-i', url]
    try:
        result = subprocess.run(command, capture_output=True, timeout=timeout,
                                stdin=subprocess.DEVNULL)
    except subprocess.TimeoutExpired:
        return dict(status='TIMEOUT')
    if result.returncode:
        # Do not leak URLs/tokens or remote text from ffprobe stderr.
        match = re.search(rb'(?:HTTP error |Server returned )(\d{3})', result.stderr)
        return dict(status='HTTP_'+match[1].decode() if match else 'PROBE_ERROR')
    try:
        return describe(json.loads(result.stdout))
    except (ValueError, TypeError, AttributeError):
        return dict(status='INVALID_RESPONSE')


def probe(url, ffprobe, timeout, attempts):
    for number in range(1, attempts+1):
        result = probe_once(url, ffprobe, timeout)
        result['attempts'] = number
        if result['status'] == 'ACTIVE':
            break
        if number < attempts:
            time.sleep(0.5)
    return result


def write_outputs(rows, results, output, report, keep_uncertain=False):
    retained = [r for r in rows if results[r['url']]['status'] == 'ACTIVE' or
                (keep_uncertain and results[r['url']]['status'] not in
                 ('HTTP_404', 'HTTP_410', 'NO_AUDIO'))]
    # Exclusive creation: never overwrite an original or previous report.
    created = []
    try:
        for path, content in (
            (output, ''.join(f'{r["name"]}\t{r["url"]}\t{r["volume"]}\n' for r in retained)),
            (report, json.dumps([dict(line=r['line'], name=r['name'],
                                     normalized=r['normalized'], **results[r['url']])
                                 for r in rows], ensure_ascii=False, indent=2)+'\n')):
            with path.open('x', encoding='utf-8', newline='') as stream:
                created.append(path)
                stream.write(content)
    except BaseException:
        for path in created:
            path.unlink(missing_ok=True)
        raise
    return len(retained)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('playlist', type=Path)
    parser.add_argument('--output', type=Path, help='Domyślnie playlist.active.csv')
    parser.add_argument('--report', type=Path, help='Domyślnie playlist.report.json')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--timeout', type=float, default=15, help='Sekundy na próbę')
    parser.add_argument('--attempts', type=int, default=2)
    parser.add_argument('--ffprobe', default='ffprobe')
    parser.add_argument('--keep-uncertain', action='store_true',
                        help='Zachowaj również timeouty i niejednoznaczne błędy sondowania')
    args = parser.parse_args(argv)
    if not 1 <= args.jobs <= 16 or not 1 <= args.attempts <= 5 or not 1 <= args.timeout <= 120:
        parser.error('jobs:1..16, attempts:1..5, timeout:1..120')
    ffprobe = shutil.which(args.ffprobe)
    if not ffprobe:
        parser.error('Brak ffprobe. Zainstaluj pakiet ffmpeg.')
    output = args.output or args.playlist.with_name(args.playlist.stem+'.active.csv')
    report = args.report or args.playlist.with_name(args.playlist.stem+'.report.json')
    paths = [p.resolve() for p in (args.playlist, output, report)]
    if len(set(paths)) != 3 or output.exists() or report.exists():
        parser.error('Pliki wyjściowe muszą być różne od wejścia i jeszcze nie istnieć.')
    try:
        rows = load_playlist(args.playlist)
        urls = list(dict.fromkeys(r['url'] for r in rows))
        print(f'Stacje: {len(rows)}, różne adresy: {len(urls)}, '
              f'oczyszczone prefiksy U+200B: {sum(r["normalized"] for r in rows)}', flush=True)
        results = {}
        pool = ThreadPoolExecutor(max_workers=args.jobs)
        try:
            futures = {pool.submit(probe, url, ffprobe, args.timeout, args.attempts): url for url in urls}
            for future in as_completed(futures):
                results[futures[future]] = future.result()
                print(f'\rSprawdzono {len(results)}/{len(urls)}', end='', flush=True)
        finally:
            pool.shutdown(wait=True, cancel_futures=True)
        print('\nSTATUS       KODEK / PROFIL          kb/s       STACJA')
        for row in rows:
            r = results[row['url']]
            codec = f'{r.get("codec", "-")} / {r.get("profile", "-")}'
            rate = f'{r["bitrate_bps"]/1000:.1f}' if r.get('bitrate_bps') else '?'
            print(f'{r["status"]:<12} {codec:<23} {rate:>7}  {row["line"]}: {row["name"]}')
        count = write_outputs(rows, results, output, report, args.keep_uncertain)
        print(f'Zapisano {count}/{len(rows)} stacji: {output}\nRaport: {report}')
        print('Bitrate z pakietów to średnia z krótkiej próbki (VBR może się zmieniać).')
        print('Błędy oznaczają wynik tej próby, nie trwałe wyłączenie stacji. Oryginał bez zmian.')
    except KeyboardInterrupt:
        parser.exit(130, '\nPrzerwano. Oryginał bez zmian.\n')
    except (OSError, ValueError) as error:
        parser.exit(2, f'Błąd: {error}\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
