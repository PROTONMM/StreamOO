# Changelog / Historia zmian

## Unreleased

- Audit remediation: reject decoded NUL in control/configuration JSON and
  bound nesting before recursive parsing; handle empty ICY input without UB.
- Add CI, repeatable host safety checks, optional parser fuzzing and contributor
  and security guidance. Correct PL/EN build instructions to use `main`.
- Full evidence and unresolved hardware checks: [audit report](docs/AUDIT_REPORT.md).

## [0.1.0](https://github.com/PROTONMM/StreamOO/releases/tag/v0.1.0) — 2026-11-14

First public development source release for ESP32-S3 N16R8 / ESP-IDF 6.1:
HTTP/HTTPS radio, MP3/AAC/FLAC/Vorbis/Opus, bounded HLS support, SSD1322 UI
and four visualizations, encoder control, local web player/settings,
persistent stations/Wi-Fi, PL/EN documentation and original-code MIT license.
Dependencies retain their separate terms. Firmware updates are wired; no OTA.
Broader hardware acceptance and long-running reliability remain open.

PL: pierwsze wydanie rozwojowe źródeł; ograniczenia i warunki zależności
opisano w README oraz instrukcjach. Zmiany audytu powyżej nie są częścią
opublikowanego tagu v0.1.0.
