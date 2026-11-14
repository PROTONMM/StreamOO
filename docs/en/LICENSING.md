# Licensing and Dependency Summary

[English index](00_INDEX.md)

Original StreamOO code, documentation, tools and artwork are covered by the root
[MIT License](../../LICENSE), copyright (c) 2026 Mariusz Księżak. The authoritative
[scope statement](../../LICENSING.md), [third-party notices](../../THIRD_PARTY_NOTICES.md)
and [license-text index](../../licenses/README.md) are already in English and apply
to this documentation as well.

## Current dependency inventory

| Material | Version/source | Terms |
| --- | --- | --- |
| ESP-IDF | 6.1 | SDK components retain their individual terms; see retained SDK copyright inventory |
| esp_audio_codec | 2.6.2 | Espressif Modified MIT, restricted to Espressif products, plus upstream notices |
| ESP-DSP | 1.8.2 | Apache-2.0 |
| mDNS | 1.14.0 | Apache-2.0 |
| ESP LittleFS adapter | 1.20.4 | MIT; bundled littlefs has BSD-3-Clause terms |
| cJSON | 1.7.19~2 | MIT |
| dr_flac | 0.13.4 | Selected MIT-0 alternative, original header retained |
| micro-flac | 0.3.0 | Apache-2.0 |
| Monaspace Krypton / StreamOO UI16 data | Source v1.400 | SIL OFL-1.1 |
| Misc Fixed Unicode 5×8 | Retained BDF revision 1.32 | Upstream public-domain declaration |
| Mozilla CA data | Retained SDK bundle source | MPL-2.0 |
| Compiler and C runtime | Actual build toolchain | Retained GCC Runtime Library Exception and runtime notices; see index |

Managed versions are pinned in component manifests and `dependencies.lock`.
The root MIT grant does not relicense libraries, fonts, CA data, reference material
or historical Git content. Original wrappers and third-party data can coexist in
one generated file while keeping their separate terms.

Choosing dr_flac for FLAC does not remove other Espressif codec/parser libraries
from the image. Do not describe the combined firmware as consisting only of
unrestricted MIT code. Host FFmpeg/ffprobe are fixture/probe tools; their presence
does not mean FFmpeg is linked into the firmware.

## Distribution

Include the root license, scope statement, third-party notices and applicable
upstream texts/NOTICE/source data for the versions actually distributed. Preserve
vendored copyright notices. Reference materials with unresolved rights remain
outside the project's MIT grant. Exclude private AP configuration, generated secret
headers, private playlists and probe reports from public source packages.

The retained license collection supplements the actual packages and build inventory.
It is not an audit of every optional SDK configuration or future dependency version.
Changing a dependency requires checking its terms and updating provenance rather
than assuming this table covers the replacement.
