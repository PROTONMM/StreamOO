# 01 — Product Scope

[English index](00_INDEX.md)

This is documentation of the current implementation, updated on 2026-11-14.
It does not imply that every hardware acceptance test has passed.

StreamOO is Internet-radio firmware for ESP32-S3-WROOM-1-N16R8 with 16 MB flash,
8 MB PSRAM, a PCM5102A I²S DAC, a 256×64 SSD1322 OLED and one rotary encoder.
It uses native ESP-IDF 6.1, without Arduino Core. yoRadio is a reference for
behavior and station-file compatibility, not an implementation or HTTP API to copy.

## Delivered functions

- HTTP/HTTPS streaming, ICY metadata, redirects and supported playlist resolution.
- MP3, AAC/HE-AAC, native FLAC, Ogg Vorbis/Opus, and a bounded HLS subset.
- Independent playback, networking and UI ownership; commands do not perform
  network I/O in encoder callbacks or display rendering.
- Encoder controls, animated station browsing, four audio visualizations,
  clock, station/track information and persistent preferences.
- A local web player, searchable full catalog, editor, import/export,
  Wi-Fi profiles, time-zone and playback settings, and browser themes.
- Build-time hardware/DSP configuration and optional diagnostics/tests.

## Deliberate boundaries

There is one station catalog and one supported encoder. The second encoder
connector in the reference schematic is not implemented. OTA is disabled:
updates use the wired flashing workflow and a single application partition.
STOP closes a stream rather than providing timeshift or recorded pause.

RC5 remote control, microSD playback and a dedicated phone application remain
future work. The phone can already use the responsive web panel. An English
documentation set does not imply an English firmware UI: the panel still uses
Polish labels, explained in the [user guide](USER_GUIDE.md).

## Completion criteria

A successful build or host test establishes only the tested behavior. Release
acceptance also needs real display/audio/network tests, restart and power-loss
checks, and long-duration playback under load. Keep unresolved evidence explicitly
OPEN; see [limitations](13_DECISIONS_OPEN.md) and the [validation matrix](14_TRACEABILITY.md).

Original application code is MIT. Third-party components and reference material
retain their own terms; see [licensing](LICENSING.md).
