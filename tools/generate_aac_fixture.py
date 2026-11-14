#!/usr/bin/env python3
"""Encode original channel cues as AAC-LC/ADTS; host tools never enter firmware."""
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import wave
from generate_mp3_fixture import synthetic_pcm
ROOT=Path(__file__).resolve().parents[1]
def main():
    rate, pcm=synthetic_pcm()
    output=ROOT/'assets/audio/channels_48k_stereo.aac'
    with tempfile.TemporaryDirectory() as directory:
        wav=Path(directory)/'source.wav'
        with wave.open(str(wav),'wb') as f:
            f.setparams((2,2,rate,0,'NONE','not compressed')); f.writeframes(pcm)
        subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-y','-i',str(wav),
            '-map_metadata','-1','-c:a','aac','-profile:a','aac_low','-b:a','128k','-f','adts',str(output)],check=True)
        raw=Path(directory)/'reference.s16le'
        subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-y','-i',str(output),
            '-f','s16le','-acodec','pcm_s16le',str(raw)],check=True)
        reference=raw.read_bytes()
    manifest={'source':'same original mathematical cues as MP3; no external recording',
        'codec':'AAC-LC','container':'ADTS','sample_rate':rate,'channels':2,'bits':16,
        'source_frames':rate*4,'reference_frames':len(reference)//4,
        'reference_pcm_sha256':hashlib.sha256(reference).hexdigest(),
        'aac_bytes':output.stat().st_size,'aac_sha256':hashlib.sha256(output.read_bytes()).hexdigest(),
        'ffmpeg':subprocess.check_output(['ffmpeg','-version'],text=True).splitlines()[0],
        'encoding':'native AAC encoder, aac_low, target128k, ADTS, no metadata',
        'peak_source_s16':1036,'edge_ramp_frames':2400}
    (ROOT/'assets/audio/channels_48k_stereo_aac.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(manifest,indent=2))
if __name__=='__main__': main()
