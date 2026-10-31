# Third-party material in StreamOO

## Small bitmap font — LICENSE REVIEW OPEN

Imported on **2026-10-24 on the user's explicit instruction** to copy it now
and document the license check as outstanding. The instruction authorizes
this project change; it is not evidence of a license from the font authors.

Source: user-provided yoRadio Polish-language ZIP, `glcdfont.c`.
Source file SHA-256:
`b837c600bebad57c18de4f5f6b2f1da6874a206a3b225d4b63e0e73e6729c7da`.

Included: 95 printable ASCII glyphs and 18 Polish glyphs, five column bytes
each, in `components/text/streamoo_font5x8.c`. Its generated lookup code is
original StreamOO code. The reference image `tests/fixtures/polish_text.4bpp`
also contains these bitmap shapes. No Arduino implementation, source
UTF-8 converter, icons or remaining extended glyphs were imported.

The source package identifies its base as Adafruit GFX. Base attribution:
Copyright (c) 2012 Adafruit Industries. The BSD notice retrieved from the
official Adafruit GFX repository is retained in
[licenses/Adafruit-GFX-BSD.txt](licenses/Adafruit-GFX-BSD.txt).
Source of that notice:
https://raw.githubusercontent.com/adafruit/Adafruit-GFX-Library/master/license.txt

The supplied ZIP does not identify the author or license of its Polish
modifications. These terms remain **UNKNOWN / TO VERIFY**. The retained BSD
notice does not establish that the Polish modifications are BSD-licensed.
The user will look for the source; update this entry when evidence is available.
No GPL/BSD/OFL license has been assigned to the unknown modifications.

Decision and exact mapping: `docs/adr/0006-provisional-polish-font-import.md`
and `docs/adr/0005-font-reference-and-utf8.md`.

## StreamOO UI16 — derived from Monaspace Krypton

Source: official Monaspace v1.400, static MonaspaceKrypton-Regular.otf.
Copyright (c) 2023 GitHub. SIL Open Font License 1.1.
The full notice, including reserved font names, is retained in
[licenses/Monaspace-OFL.txt](licenses/Monaspace-OFL.txt).

The source OTF is unmodified. The rasterized ASCII/Polish subset has the
separate primary name **StreamOO UI16**, retaining source attribution. Font
data and their converted subset remain OFL-1.1; this does not change the
license of the original renderer code or resolve the small PL font's status.

Files: `assets/fonts/MonaspaceKrypton-Regular.otf`, source manifest,
`components/text/generated/streamoo_ui_16.h`, and the reference image
`tests/fixtures/home_*.4bpp`. Source hash, release URL and exact
regeneration process are documented in `assets/fonts/README.md`.

The station-list reference `tests/fixtures/station_list.4bpp` contains the same font shapes described above: StreamOO UI16 (OFL-1.1) and the user-authorized small PL font with modification license still under review.

## Espressif audio codec 2.6.2 — MP3/AAC-LC PoC

Pinned managed component, used only on ESP32-S3 under **Espressif Modified
MIT** (Espressif-only restriction, not unrestricted MIT). Complete license:
[Espressif notice](licenses/Espressif-Audio-Codec-Modified-MIT.txt).
Vendor attribution for MP3 and AAC: Android Open Source Project2005–2008, Apache2.0;
[Apache text](licenses/Apache-2.0.txt) and [version/audit/source details](licenses/ESP-Audio-Codec-NOTICE.md).
MP3 and AAC-LC/ADTS are supported by the adapter; only the selected codec
is registered. AAC-Plus is disabled; HE-AAC remains unverified.
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
