#!/usr/bin/env python3
"""Generate original synthetic MP3; FFmpeg is a host tool, never target code."""
import hashlib
import json
import math
from pathlib import Path
import subprocess
import tempfile
import wave

ROOT = Path(__file__).resolve().parents[1]

def main():
    rate = 48000
    pcm = bytearray()
    for index in range(rate*4):
        left = right = 0
        for start,end,channels,hz in ((24000,72000,1,1000),(96000,144000,2,2000),(144000,168000,3,1000)):
            if start <= index < end:
                local = index-start
                envelope = min(1, local/2400, (end-1-index)/2400)
                value = round(1036*envelope*math.sin(2*math.pi*hz*local/rate))
                left = value if channels & 1 else 0
                right = value if channels & 2 else 0
        pcm.extend(left.to_bytes(2,'little',signed=True))
        pcm.extend(right.to_bytes(2,'little',signed=True))
    out = ROOT/'assets/audio/channels_48k_stereo.mp3'
    with tempfile.TemporaryDirectory() as directory:
        wav = Path(directory)/'original.wav'
        with wave.open(str(wav),'wb') as f:
            f.setparams((2,2,rate,0,'NONE','not compressed')); f.writeframes(pcm)
        cmd = ['ffmpeg','-hide_banner','-loglevel','error','-y','-i',str(wav),
               '-map_metadata','-1','-c:a','libmp3lame','-b:a','128k',
               '-write_xing','0','-id3v2_version','0',str(out)]
        subprocess.run(cmd,check=True)
        raw = Path(directory)/'reference.s16le'
        subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-y','-i',str(out),
                        '-f','s16le','-acodec','pcm_s16le',str(raw)],check=True)
        data = raw.read_bytes()
    manifest = {'source':'original mathematical signals; no external recording',
                'sample_rate':rate,'channels':2,'bits':16,'source_frames':rate*4,
                'reference_frames':len(data)//4,'reference_pcm_sha256':hashlib.sha256(data).hexdigest(),
                'mp3_bytes':out.stat().st_size,'mp3_sha256':hashlib.sha256(out.read_bytes()).hexdigest(),
                'ffmpeg':subprocess.check_output(['ffmpeg','-version'],text=True).splitlines()[0],
                'encoding':'libmp3lame 128k CBR, write_xing=0, id3v2_version=0, no metadata',
                'sequence_seconds':'0-.5 zero; .5-1.5 L1k; 1.5-2 zero; 2-3 R2k; 3-3.5 both1k; 3.5-4 zero',
                'peak_source_s16':1036,'edge_ramp_frames':2400}
    (ROOT/'assets/audio/channels_48k_stereo.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(manifest,indent=2))
if __name__=='__main__': main()
