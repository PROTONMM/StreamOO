#!/usr/bin/env python3
"""Generate original signals for the ESP32 extended-codec hardware acceptance.
FFmpeg/ffprobe run on the host only; no external recordings or codec code copied.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
from urllib.parse import urlsplit
import wave
from generate_mp3_fixture import synthetic_pcm


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, required=True, help='New directory; never overwrite an existing one')
    parser.add_argument('--base-url', required=True, help='HTTP address of this computer as seen by the radio, e.g. http://192.168.1.20:8000')
    args = parser.parse_args()
    base = args.base_url.rstrip('/')
    url = urlsplit(base)
    if url.scheme not in ('http', 'https') or not url.hostname or url.username or url.password or url.query or url.fragment or any(c.isspace() for c in base):
        parser.error('Provide a plain HTTP(S) server base URL without credentials/query/fragment')
    args.output_dir.mkdir(parents=True, exist_ok=False)
    rate, pcm = synthetic_pcm()
    variants = [
        ('MP3 reference', 'reference.mp3', ['-c:a', 'libmp3lame', '-b:a', '128k', '-write_xing', '0', '-id3v2_version', '0']),
        ('AAC-LC reference', 'reference.aac', ['-c:a', 'aac', '-profile:a', 'aac_low', '-b:a', '128k', '-f', 'adts']),
        ('FLAC 16-bit', 'signal16.flac', ['-c:a', 'flac', '-sample_fmt', 's16']),
        ('FLAC 24-bit', 'signal24.flac', ['-c:a', 'flac', '-sample_fmt', 's32']),
        ('Ogg Vorbis', 'signal.ogg', ['-c:a', 'libvorbis', '-q:a', '4']),
        ('Ogg Opus 20ms', 'signal.opus', ['-c:a', 'libopus', '-b:a', '96k', '-frame_duration', '20']),
        ('Ogg Opus 120ms', 'long-frame.opus', ['-c:a', 'libopus', '-b:a', '96k', '-frame_duration', '120']),
    ]
    manifest = {'source': 'Original mathematical L1k/R2k signals with gaps and 50ms ramps, source peak1036/32768; repeated to32s', 'files': []}
    hls_entries=[]
    with tempfile.TemporaryDirectory() as tmp:
        wav = Path(tmp)/'source.wav'
        with wave.open(str(wav), 'wb') as f:
            f.setparams((2, 2, rate, 0, 'NONE', 'not compressed'))
            f.writeframes(pcm*8)
        for name, filename, options in variants:
            output = args.output_dir/filename
            subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-n', '-i', str(wav), '-map_metadata', '-1', *options, str(output)], check=True,timeout=30)
            info = json.loads(subprocess.check_output(['ffprobe', '-v', 'error', '-show_streams', '-of', 'json', str(output)],timeout=20))
            manifest['files'].append({'name': name, 'file': filename, 'sha256': hashlib.sha256(output.read_bytes()).hexdigest(), 'streams': info['streams']})
        for label,stem,codec,container in [('HLS AAC MPEG-TS','hls-aac-ts','aac','mpegts'),('HLS MP3 MPEG-TS','hls-mp3-ts','libmp3lame','mpegts'),('HLS AAC packed','hls-aac-packed','aac','adts')]:
            directory=args.output_dir/stem;directory.mkdir();output=directory/'index.m3u8'
            subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-n','-i',str(wav),'-map','0:a:0','-map_metadata','-1','-c:a',codec,'-b:a','128k','-f','segment','-segment_time','4','-segment_format',container,'-segment_list',str(output),'-segment_list_type','m3u8',str(directory/('segment%03d.ts' if container=='mpegts' else 'segment%03d.aac'))],check=True,timeout=30)
            info=json.loads(subprocess.check_output(['ffprobe','-v','error','-show_streams','-of','json',str(output)],timeout=20))
            manifest['files'].append({'name':label,'file':stem+'/index.m3u8','streams':info['streams']})
            hls_entries.append((label,base+'/'+stem+'/index.m3u8'))
    (args.output_dir/'relative.m3u').write_text('#EXTM3U\n#EXTINF:-1,FLAC fixture\nsignal16.flac\n')
    (args.output_dir/'relative.pls').write_text('[playlist]\nFile2=reference.mp3\nFile1=signal.ogg\nNumberOfEntries=2\nVersion=2\n')
    (args.output_dir/'nested.m3u').write_text('#EXTM3U\nrelative.pls\n')
    (args.output_dir/'reject-hls.m3u8').write_text('#EXTM3U\n#EXT-X-KEY:METHOD=AES-128,URI="key"\n#EXT-X-TARGETDURATION:6\n#EXTINF:6,\nnot-a-segment.ts\n')
    entries = [(name, base+'/'+filename) for name, filename, _ in variants]
    entries += hls_entries
    entries += [('M3U relative - FLAC', base+'/relative.m3u'), ('PLS File1 - Vorbis', base+'/relative.pls'), ('Nested M3U to PLS', base+'/nested.m3u'), ('HE-AAC Kiss FM', 'https://streaming.g-news.pl:8090/kissfm'), ('AAC+ Nowy Swiat', 'https://stream.nowyswiat.online/aac')]
    with (args.output_dir/'playlist.csv').open('w') as f:
        for name, address in entries:
            f.write(f'{name}\t{address}\t0\n')
    (args.output_dir/'manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2)+'\n')
    print(f'Generated {len(variants)} audio files, 3 HLS fixtures and playlist in {args.output_dir}')
    print(f'Serve with: python3 -m http.server 8000 --bind 0.0.0.0 --directory {args.output_dir}')
    print('Each synthetic file is32s, then EOF/retry is expected. HE-AAC entries are public live stations, not synthetic fixtures.')

if __name__ == '__main__':
    main()
