# Espressif audio codec used by StreamOO

Component: espressif/esp_audio_codec 2.6.2, pinned in idf_component.yml and
root dependencies.lock. Registry component hash:
a5b9275af74247e3ce536d16a7080e4d77207a185b63e1bea368eb4d57176c2b.
Archive records upstream commit f9110cbb471b4017b0faf9bd48fcac84c61ecb6d.

The component is supplied under Espressif Modified MIT, copyright2025
Espressif Systems (Shanghai) CO., LTD. The complete notice is retained in
Espressif-Audio-Codec-Modified-MIT.txt. It permits use only with Espressif
products and prohibits redistribution for non-Espressif products. This is
not unrestricted MIT. StreamOO targets ESP32-S3; the vendor binary is not
used in native host tests, which test only our original adapter/model code.

The vendor's official copyright list attributes the MP3 and AAC implementations to
The Android Open Source Project, copyright2005–2008, under Apache License2.0.
Retain Apache-2.0.txt alongside this notice. The adapter supports MP3 and AAC-LC/ADTS. Both backends are linked in file
fixture builds; only the selected codec is registered. AAC-Plus is disabled.
Other codecs and HE-AAC have not been validated by StreamOO.
No GPL code was imported into target firmware by this integration.

Sources checked2026-10-25:
- https://components.espressif.com/components/espressif/esp_audio_codec/versions/2.6.2/license
- https://docs.espressif.com/projects/esp-adf/en/latest/COPYRIGHT.html
- Package LICENSE/idf_component.yml and public headers, downloaded from the official registry.

Dependencies are installed unmodified by ESP-IDF Component Manager into
ignored managed_components; firmware distributions need the retained notices.
The original generated MP3/AAC signals is not third-party recorded material.

## Extended decoding integration (2026-10-31)

Same unmodified, pinned2.6.2 binaries; no dependency upgrade. The adapter now
also enables AAC-Plus (HE-AAC/HE-AACv2), native FLAC, and Ogg Vorbis/Opus.
This supersedes the historical AAC-LC-only scope above. Native decoder
execution/quality/performance still require ESP32-S3 hardware acceptance.
The vendor API supplies arbitrary-fragment FLAC/Ogg parsing; Ogg registration
is explicit alongside only the requested raw codec. No default-all registry.

Reviewed public headers (esp_flac_dec, esp_vorbis_dec, esp_opus_dec,
impl/esp_ogg_dec), package LICENSE and vendor copyright list before linking
these backends. Official attribution:
https://docs.espressif.com/projects/esp-adf/en/latest/COPYRIGHT.html

Retained upstream license texts linked by that list:
- Opus: [Opus-COPYING.txt](Opus-COPYING.txt), BSD3 and associated patent terms,
  source https://github.com/xiph/opus/blob/v1.3.1/COPYING .
- FLAC decoder: [FLAC-BSD.txt](FLAC-BSD.txt), Xiph BSD,
  source https://github.com/xiph/flac/blob/1.3.4/COPYING.Xiph .
- Vorbis: [Vorbis-BSD.txt](Vorbis-BSD.txt), BSD3,
  source https://github.com/xiph/vorbis/blob/v1.3.7/COPYING .

These are retained license/attribution references, not claims of the exact
upstream implementation revisions embedded in vendor binaries. The vendor
manifest/archive hash above identifies what StreamOO actually links. No
FLAC command-line tools (GPL) or other GPL implementation copied to firmware.
The optional fixture generator invokes installed FFmpeg/ffprobe as host
processes only, with original mathematical signals.

ADR0060 also registers this same pinned component's MPEG-TS simple decoder
(`impl/esp_ts_dec.h`) with MP3 and AAC/AAC-plus; no new archive/version.
HLS playlist/network handling is original StreamOO code, not vendor code.
