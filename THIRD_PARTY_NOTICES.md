# Third-party material in StreamOO

Current release-license assessment: [MIT feasibility review](docs/LICENSING_REVIEW.md).
Original application licensing does not replace the licenses below.
The small font was replaced with public-domain Misc Fixed Unicode 5x8 in ADR0102.

## ESP-DSP 1.8.2 — vector FFT on ESP32-S3

Pinned managed component `espressif/esp-dsp`, Apache License 2.0,
Copyright Espressif Systems (Shanghai) PTE LTD. License, public API,
target assembly and IDF requirement (`>=4.2`) reviewed before integration
with ESP-IDF 6.1. Sources remain unmodified and are supplied by the component
manager. Retained [Apache license](licenses/Apache-2.0.txt) and
[version/source notice](licenses/ESP-DSP-NOTICE.md). No GPL code imported.

The FFT adapter calls the AES3 sc16 kernel explicitly; host tests use the
same version's ANSI sc16 implementation as a numerical oracle. They do not
execute or benchmark the ESP32-S3 instructions.

## Small bitmap font — Misc Fixed Unicode 5x8 (public domain)

Source: Markus Kuhn's X11 Unicode font archive, `5x8.bdf` revision1.32
(2006-01-05). Retained unmodified at `assets/fonts/misc-fixed/5x8.bdf`.
Source SHA-256: `a126e263c6ee843e8725fa3ad630879e74d3294493ce7a00f9a9a21ae10fc2f3`.
Source declaration: public domain; provenance and upstream URLs are retained
in [Misc Fixed notice](licenses/Misc-Fixed-NOTICE.md).

Included:95 printable ASCII and18 Polish glyphs, five column bytes each,
in `components/text/streamoo_font5x8.c`. Converter and lookup are original
StreamOO code. No Arduino/X11 runtime implementation is imported. The six
font-bearing framebuffer fixtures were regenerated and reviewed for ADR0102.

This replaces the provisional user-provided font from ADR0006. Unknown
rights to its Polish modifications no longer apply to the current font
subset or regenerated fixtures. Historical Git revisions and reference
materials retain their previous status; they are not relicensed here.

## StreamOO UI16 — derived from Monaspace Krypton

Source: official Monaspace v1.400, static MonaspaceKrypton-Regular.otf.
Copyright (c) 2023 GitHub. SIL Open Font License 1.1.
The full notice, including reserved font names, is retained in
[licenses/Monaspace-OFL.txt](licenses/Monaspace-OFL.txt).

The source OTF is unmodified. The rasterized ASCII/Polish subset has the
separate primary name **StreamOO UI16**, retaining source attribution. Font
data and their converted subset remain OFL-1.1; this does not change the
license of the original renderer code.

Files: `assets/fonts/MonaspaceKrypton-Regular.otf`, source manifest,
`components/text/generated/streamoo_ui_16.h`, and the reference image
`tests/fixtures/home_*.4bpp`. Source hash, release URL and exact
regeneration process are documented in `assets/fonts/README.md`.

The station-list reference `tests/fixtures/station_list.4bpp` contains the same font shapes described above: StreamOO UI16 (OFL-1.1) and public-domain Misc Fixed Unicode 5x8.

## Espressif audio codec 2.6.2 — MP3/AAC-LC PoC

Pinned managed component, used only on ESP32-S3 under **Espressif Modified
MIT** (Espressif-only restriction, not unrestricted MIT). Complete license:
[Espressif notice](licenses/Espressif-Audio-Codec-Modified-MIT.txt).
Vendor attribution for MP3 and AAC: Android Open Source Project2005–2008, Apache2.0;
[Apache text](licenses/Apache-2.0.txt) and [version/audit/source details](licenses/ESP-Audio-Codec-NOTICE.md).
The adapter supports MP3, AAC-LC/HE-AAC, FLAC and Ogg Vorbis/Opus;
registration is limited to the requested format/container. The earlier
AAC-LC-only scope is superseded by the extended integration described below.
The Espressif restriction still applies when dr_flac is selected for FLAC.
Host tests do not use the vendor binaries. FFmpeg/libmp3lame are host fixture
generation/reference tools, not linked target dependencies. No GPL implementation
is copied to firmware. The synthetic MP3/AAC fixtures contain no external recording.

## Native ESP-IDF Wi-Fi09a

esp_wifi/esp_netif/esp_event from pinned ESP-IDF6.1; local public headers and
esp_wifi/lib/LICENSE reviewed as Apache2.0. Retained Apache-2.0.txt and
[Wi-Fi SDK notice](licenses/ESP-IDF-WiFi-NOTICE.md). No SDK code copied.

09b also uses native esp_http_server from the same pinned SDK, public header
copyright2018–2026 Espressif Systems, Apache-2.0. Retained Apache-2.0.txt.
Portal HTML/CSS/JS and OLED layout are original; no external frontend assets.

## Configuration storage and JSON (09c)

Exact managed versions: **joltwallet/littlefs 1.20.4** and
**espressif/cjson 1.7.19~2**, pinned in dependencies.lock. API/manifests and
upstream license files checked before integration on ESP-IDF 6.1.
LittleFS adapter: MIT, Brian Pugh 2020; bundled littlefs: BSD-3-Clause,
littlefs authors 2022 and Arm Limited 2017. cJSON: MIT, Dave Gamble 2009–2017.
Unmodified sources supplied by the component manager, no GPL code imported.
Retained texts: [adapter MIT](licenses/ESP-LittleFS-MIT.txt),
[littlefs BSD](licenses/LittleFS-BSD-3-Clause.txt), [cJSON MIT](licenses/cJSON-MIT.txt).
The native SDK mbedtls base64 decoder is used for HTTP Basic setup proof;
no third-party authentication or frontend code copied.

## Native live HTTP/ICY player 06i

Uses pinned ESP-IDF6.1 `esp_http_client`/`esp_netif` (Apache-2.0), SDK HTTP
Parser2.7.0 (MIT), and Mbed TLS4.1.0 under its Apache-2.0 option. Full SDK
Mozilla CA data is MPL-2.0; retained license and corresponding PEM source.
[Versions, source availability and licenses](licenses/ESP-HTTP-TLS-NOTICE.md).
Transport, ICY filter, linear PCM conversion and player integration are
original StreamOO implementations. Existing Espressif codec2.6.2 terms apply
unchanged. No yoRadio or other GPL implementation copied.

## LAN service discovery (09f)

espressif/mdns **1.14.0**, Apache-2.0, exact registry hash pinned in
`dependencies.lock`; native ESP-IDF6.1 build checked. Public API and
upstream LICENSE reviewed before integration. Retain
[license and source notice](licenses/ESP-mDNS-NOTICE.md) and Apache-2.0.txt.

## Extended audio codecs (06k)

Existing pinned espressif/esp_audio_codec2.6.2 now used for AAC-Plus,
FLAC, Ogg Vorbis and Ogg Opus. Espressif Modified MIT (Espressif products
only), with vendor-attributed Xiph BSD backends. See expanded
[codec notice](licenses/ESP-Audio-Codec-NOTICE.md) and retained Opus/FLAC/
Vorbis license texts. No external codec implementation copied.

## Selectable FLAC comparison backends (ADR0078)

micro-flac0.3.0, Apache-2.0, decoder-only source subset including original
Xtensa LPC assembly; encoder and Ogg submodule excluded. dr_flac0.13.4,
MIT-0 selected, original single header including both license texts retained.
Exact commits, per-file SHA-256 and scope are recorded in
[third-party origins](components/decoder/third_party/ORIGINS.json) and
[source notice](components/decoder/third_party/README.md).
micro-flac license is retained at components/decoder/third_party/micro-flac/LICENSE.
No GPL imported; both upstream implementations are unmodified. Native IDF6.1
builds verified; physical S3 performance and playback remain OPEN.

## HLS / MPEG-TS — ADR0060

HLS manifests, sequence handling, ID3 prefix stripping and HTTP segment
orchestration are original StreamOO code based on RFC8216, with no imported
HLS implementation. MPEG-TS uses `esp_ts_dec` in the existing pinned
Espressif audio codec2.6.2 under the already retained Espressif Modified MIT
terms; underlying MP3/AAC registrations retained. API/configuration/license
checked in the pinned headers. No new dependency or GPL code imported.

## Toolchain runtime in the linked image

The application map also includes libgcc/libstdc++ runtime and Picolibc
libc/libm. GCC's applicable Runtime Library Exception3.1 permits independent
non-GPL applications under its conditions; this is separate from importing
a GPL codec without an exception. Picolibc carries per-file BSD/MIT-family
notices. These runtimes must be included in the release-license inventory;
the compiler's own license does not automatically apply to the whole app.
See [full module and replacement analysis](docs/MIT_MIGRATION_ANALYSIS.md)
and its local toolchain notice paths. Statements above about no imported GPL
codec implementations do not claim that every runtime file is MIT.
