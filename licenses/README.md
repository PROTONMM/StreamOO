# Retained licenses and notices

The original StreamOO application uses the root [MIT license](../LICENSE).
[LICENSING.md](../LICENSING.md) defines its scope and exceptions. Files in this
directory retain their upstream terms; they are not relicensed as StreamOO code.
Component versions, origins and choices are in [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md).

| Material | Retained text / attribution |
| --- | --- |
| Espressif audio codecs 2.6.2 | [Modified MIT](Espressif-Audio-Codec-Modified-MIT.txt), [codec notice](ESP-Audio-Codec-NOTICE.md); restricted to Espressif products |
| Apache components: ESP-IDF, ESP-DSP, mDNS, micro-flac and Apache option of Mbed TLS | [Apache-2.0](Apache-2.0.txt), [DSP notice](ESP-DSP-NOTICE.md), [mDNS notice](ESP-mDNS-NOTICE.md), [Wi-Fi notice](ESP-IDF-WiFi-NOTICE.md), [HTTP/TLS notice](ESP-HTTP-TLS-NOTICE.md); retain component-specific notices too |
| ESP-IDF license overview | [Unmodified SDK copyright inventory](ESP-IDF-COPYRIGHT.rst); Sphinx references refer to the SDK, not this directory |
| FreeRTOS, lwIP, wpa_supplicant | [FreeRTOS MIT](FreeRTOS-MIT.txt), [lwIP notice](lwIP-COPYING.txt), [wpa_supplicant notice](wpa_supplicant-COPYING.txt) |
| GCC runtime (libgcc/libstdc++) | [GPLv3](GCC-GPL-3.0.txt) together with [GCC Runtime Library Exception 3.1](GCC-Runtime-Exception-3.1.txt); applicable runtime terms, not a change to the application's MIT license |
| Picolibc/Newlib runtime | [Picolibc file-level inventory](Picolibc-COPYING.txt), [Newlib notices](Newlib-COPYING.txt) |
| dr_flac | [Selected MIT-0 alternative](dr_flac-MIT-0.txt); original header and provenance remain in [third_party](../components/decoder/third_party/README.md) |
| LittleFS and adapter | [BSD-3-Clause](LittleFS-BSD-3-Clause.txt), [adapter MIT](ESP-LittleFS-MIT.txt) |
| cJSON and HTTP Parser | [cJSON MIT](cJSON-MIT.txt), [HTTP Parser MIT](HTTP-Parser-MIT.txt) |
| Codec upstream attributions | [FLAC](FLAC-BSD.txt), [Vorbis](Vorbis-BSD.txt), [Opus](Opus-COPYING.txt) |
| Monaspace Krypton and derived glyph data | [SIL OFL-1.1](Monaspace-OFL.txt) |
| Current small font: Misc Fixed Unicode 5x8 | [Public-domain declaration and provenance](Misc-Fixed-NOTICE.md) |
| Mozilla CA bundle | [MPL-2.0](MPL-2.0.txt), [corresponding PEM source](sources/cacrt_all.pem), [source notice](ESP-HTTP-TLS-NOTICE.md) |
| Earlier Adafruit-based bitmap font | [Historical BSD notice](Adafruit-GFX-BSD.txt); retained for provenance, not the license of the current Misc Fixed data |

The added SDK/runtime copies are byte-for-byte from the pinned local SDK and
compiler distribution. [UPSTREAM_SOURCES.json](UPSTREAM_SOURCES.json) records
the source package/path and SHA-256 of each added text. The dr_flac license is
the selected text extracted from its retained header without the C comment
delimiter; its upstream copyright statement is unchanged.

The complete Picolibc inventory also describes source-package files and host
tools not linked into StreamOO. A license mentioned in that inventory is not,
by itself, evidence that code under it occurs in the firmware. Likewise, the
SDK overview describes optional components. Exact per-file terms take priority
over a summary; match notices to the versions and components actually shipped.

For a release, keep these files and applicable upstream LICENSE/NOTICE files
with the source/firmware, together with the root LICENSE, LICENSING.md and
THIRD_PARTY_NOTICES.md. SDK or dependency upgrades require refreshing the
corresponding notices and CA source where applicable. This collection does
not grant new rights to reference materials or replace the original licenses.
