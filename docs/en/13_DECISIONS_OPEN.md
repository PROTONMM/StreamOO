# 13 — Known Limitations and Outstanding Validation

[English index](00_INDEX.md)

This page describes the current implementation as of 2026-11-14. It replaces neither
the original development evidence nor a release-specific hardware report. The Polish
archive is not translated as part of this documentation set.

## Established product decisions

- Native ESP-IDF 6.1 on ESP32-S3 N16R8; one encoder, SSD1322 and PCM5102A.
- Actual encoder wiring 5/6/7, despite B=16 in the older reference profile.
- One catalog; import appends; stable IDs survive editor name/URL changes.
- AP is for configuration, with audio disabled. The local panel has no separate login.
- A shared build-time AP password and unique MAC-derived AP/mDNS names.
- Four visualizations, including two maximum spectra with the same −45…0 dBFS scale.
  The fourth has peak hold; it is not an RMS spectrum.
- Autoplay is configurable, enabled by default; a saved current-format STOP setting
  remains respected. No arbitrary fallback station if the saved ID is absent.
- One large application partition, wired updates and no OTA.
- MIT for original work; third-party terms remain unchanged. The current small font
  has public-domain provenance; the removed font's historical rights are not retroactively resolved.

## Unsupported or future functions

| Area | Boundary |
| --- | --- |
| Controls | No RC5 implementation, second encoder or dedicated phone app |
| Sources | No microSD playback or timeshift |
| HLS | No encryption/DRM, fMP4, BYTERANGE or LL-HLS; discontinuity after start ends the session |
| Audio formats | No declared multichannel, Ogg-FLAC/chained Ogg or raw Opus/RTP support; live input limited to supported mono/stereo 8–48 kHz |
| Web | Polish firmware labels; English docs are not UI localization; editor retains 25-row pages |
| Security | Local HTTP without user authentication, no normal-profile Secure Boot or Flash Encryption |
| Clock | Fixed supported zone list and encoded DST rules, not a full runtime time-zone database |

## Validation still requiring care

Each codec/profile and stream combination requires its own listening test. Previous
successful FLAC playback does not prove every server or FLAC block size works. The
history includes delivery underruns, parser recovery and TLS/crypto failures; the
current defaults and fixes do not establish that all causes are eliminated.

Long playback with web activity, rapid station changes, TLS/HLS transitions,
network loss, minimum/largest internal heap and stack margin needs measurements
under the actual build. A historical log with no underruns is workload-specific.
Large free PSRAM does not prove enough contiguous internal memory is available.

Physical power interruption during NVS/LittleFS writes, different routers/mDNS
clients, multiple-device discovery and OLED scan/brightness/readability require
hardware evidence. The user accepted the smaller font with a readability reservation
and reported the compact web controls/live volume working well; this is not a full
matrix of phones, browsers and hardware revisions.

Host FFT comparisons confirm numerical behavior. They do not establish a controlled
SIMD speedup under equal board conditions. The metering task intentionally drops
blocks under load; distinguish that from audible output failure.

## Recording a result

Use [testing and delivery](12_TESTS_DELIVERY.md). Include revision, configuration,
test source, duration, relevant counters and a clear distinction between measured,
heard, inferred and untested behavior. Keep unresolved issues OPEN and do not copy
old acceptance statements forward as proof for a new configuration.
