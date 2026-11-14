# Format, Stream and Playlist Validation

[English index](00_INDEX.md)

This is a procedure for the current firmware, not a translated history of earlier
codec experiments. Run the tests on the exact firmware/profile you plan to use.
Host decoding and network probing do not establish that the ESP32 can play a source.

## Supported scope

| Source | Current implementation and limits |
| --- | --- |
| MP3 | HTTP/HTTPS and supported playlist/HLS transport |
| AAC | ADTS AAC-LC/HE-AAC family through the registered decoder; verify each profile |
| FLAC | Native FLAC, including supported live input without the file header; selectable dr/micro/Espressif backends |
| Ogg | Vorbis and Opus; Ogg-FLAC/chained Ogg are not declared supported |
| M3U/PLS | A station URL may resolve to a playable stream; this does not replace station-catalog import |
| HLS | Unencrypted master/media playlists, supported packed MP3/AAC and MPEG-TS audio |

Live output accepts supported mono/stereo 8–48 kHz formats and converts to the
fixed 48 kHz stereo output. Multichannel and raw Opus/RTP are not declared supported.
HLS encryption/DRM, fMP4, BYTERANGE and LL-HLS are unsupported; discontinuity after
startup ends a session. Current HLS parsing limits include 16 KiB manifests and
32 segments. M3U/PLS resolution is bounded in document size/depth and redirects.

## Checking a station list

Run from the repository root:

```sh
python3 tools/check_playlist.py playlist.csv
```

This is a read-only format audit. It reports line numbers and codes without station
names/URLs. Exit 1 indicates validation issues. Duplicate URLs produce warnings.
The script is not a firmware upload and does not modify the private original file.

For source availability and format probing, install host FFmpeg/ffprobe and use:

```sh
python3 tools/probe_playlist.py playlist.csv
```

The tool uses Python 3.9+ and `ffprobe` on PATH. By default it uses four concurrent
processes, two attempts per URL, 15 seconds per attempt and 200 packets per sample.
Repeated URLs are probed once, but entry order, duplicates and `ovol` are retained.
It creates new `playlist.active.csv` and `playlist.report.json` files alongside the
input without overwriting it. Keep these private, including when using custom names.

The report distinguishes HTTP 404/410, timeout, no audio and probe failures. A failure
means the source could not be validated from that computer at that time, not that
the station is permanently gone. The bitrate estimate uses audio packet sizes/time
or a reported positive stream bitrate, not the URL or MIME name.

```sh
python3 tools/probe_playlist.py playlist.csv --keep-uncertain --timeout 20 --attempts 3
python3 tools/probe_playlist.py playlist.csv --output playlist.active.2.csv --report playlist.report.2.json
```

`--keep-uncertain` retains ambiguous results, but not confirmed 404/410/no-audio
entries. Output filenames must be new. ACTIVE means ffprobe recognized audio on
the host; it does not guarantee firmware format/transport compatibility. Review
the output before importing it. Import will append, not replace existing stations.

## Controlled LAN fixtures

Use a computer and the radio on the same LAN. Choose the computer's address that
the radio can reach; `192.168.1.20` below is only an example. The generators require
host FFmpeg/ffprobe and create a new output directory rather than overwrite one.

For the extended codec/HLS fixtures:

```sh
python3 tools/generate_extended_fixtures.py \
  --output-dir build/format-lan-test \
  --base-url http://192.168.1.20:8000
python3 -m http.server 8000 --bind 0.0.0.0 --directory build/format-lan-test
```

Leave the server terminal open, allow LAN access to port 8000 and import the generated
playlist through the radio's web interface. Export your catalog first if you want
a baseline backup. The generator creates original channel-identification signals
with intentional silent intervals and finite duration. EOF/retry at the end is
expected; do not count designed silence as a dropout. It includes direct formats,
M3U/PLS resolution and supported local HLS variants.

For longer FLAC delivery tests:

```sh
python3 tools/generate_flac_network_fixture.py \
  --output-dir build/flac-lan-test \
  --base-url http://192.168.1.20:8000 \
  --seconds 300
python3 -m http.server 8000 --bind 0.0.0.0 --directory build/flac-lan-test
```

Test the 16-bit tone, 24-bit noise and 24-bit headerless-live entries separately.
Listen for at least three minutes per entry and record reserve/underrun/recovery
counters. Files last 300 seconds in this example; EOF/retry is normal at the end.
The HTTP server does not declare ICY bitrate, so the radio can show a measured
estimate after enough decoded audio. Stop the test server with Ctrl+C afterwards.

A successful LAN test removes Internet/TLS/ICY variables simultaneously. It does
not identify which of them caused a public-stream failure. Keep generated media,
playlists and reports under ignored `build/`, not in tracked source fixtures.

## Comparing FLAC backends

The normal backend is dr_flac. To build explicit alternatives:

```sh
python3 tools/build_flac.py dr build
python3 tools/build_flac.py micro build
python3 tools/build_flac.py esp build
```

Run these sequentially. Each uses a separate persistent configuration and build
directory. Flash the selected variant explicitly, for example:

```sh
python3 tools/build_flac.py dr flash --port /dev/ttyUSB0
```

Inspect its GPIO and build settings before flashing. The wrapper checks the generated
configuration and backend marker; a directory name alone does not select a decoder.
It uses the ESP-IDF Python environment even if launched by another Python.

Optional controlled experiments include `--software-crypto`, `--hardware-crypto`
and `--tcp-window 23040`. Hardware/software crypto options are mutually exclusive.
The chosen settings persist in that backend's private configuration. These experiments
do not disable TLS verification. Change one relevant variable at a time and document
the baseline rather than comparing unrelated configurations.

## HLS and public-source acceptance

Test MP3, AAC-LC and each needed HE-AAC profile separately, through multiple segment
boundaries and manifest reloads. Include STOP/PLAY, switching away and back, and
network loss during a segment. Session query parameters must survive reloads.
Record the actual detected codec/sample rate instead of relying on station names.

The [original HLS report](../HLS_BRINGUP.md) and
[FLAC comparison reports](../FLAC_BACKENDS_BRINGUP.md) remain in Polish as archived
evidence. Their old firmware IDs and old tuning proposals are not instructions to
downgrade the current configuration. Current unresolved limits are listed in
[13_DECISIONS_OPEN.md](13_DECISIONS_OPEN.md).
