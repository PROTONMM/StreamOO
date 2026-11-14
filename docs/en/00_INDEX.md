# StreamOO — English Documentation

[English README](../../README_EN.md) · [Polski](../00_INDEX.md)

Updated: 2026-11-14. This directory contains complete operating/build guides and
current technical documentation. At the user's request, the chronological history,
old reports and full ADR archive remain in Polish. Historical links are labeled;
they are supporting evidence, not untranslated required steps in the guides.

“Current documentation” describes implemented behavior and explicit limitations.
It does not mean every hardware acceptance item has passed. The firmware web UI
still uses Polish labels, with English explanations in the user guide.

## Start here

| Document | Use it for |
| --- | --- |
| [User guide](USER_GUIDE.md) | First startup, Wi-Fi, encoder, OLED, web player, playlists and troubleshooting |
| [Build guide](BUILD_GUIDE.md) | ESP-IDF setup, private AP password, every main menuconfig group, building, flashing and diagnostics |
| [Format validation](FORMAT_VALIDATION.md) | Playlist probing, controlled LAN fixtures, FLAC comparisons and HLS checks |
| [Current status](STATUS.md) | Implemented features, evidence and limits |
| [Development entry point](RESUME.md) | Where to begin maintenance work |
| [Contribution guidance](CONTRIBUTING.md) | Engineering rules, asset generation, bilingual maintenance and commit policy |
| [Licensing](LICENSING.md) | MIT scope, dependency versions, exceptions and distribution notices |

## Technical reference

| Chapter | Scope |
| --- | --- |
| [01 — Product scope](01_SCOPE.md) | Delivered functions and product boundaries |
| [02 — Hardware](02_HARDWARE.md) | Wiring, panel/DAC, GPIO policy and board acceptance |
| [03 — Platform and architecture](03_PLATFORM.md) | Components, ownership, command/data flow, cores and memory |
| [04 — Display and UI](04_DISPLAY_UI.md) | OLED layout, drawing, fonts, startup and visualizations |
| [05 — Encoder](05_ENCODER.md) | Input capture, modes, gestures and selection semantics |
| [06 — Audio](06_AUDIO.md) | Codecs, actual player phases, PCM, buffering, gain and bitrate |
| [07 — DSP](07_DSP.md) | Hann/FFT, RMS/peak, SIMD, display mapping and validity |
| [08 — Stations](08_STATIONS.md) | File format, identity, import/export, storage and host tools |
| [09 — Networking](09_NETWORK.md) | STA/AP, mDNS, SNTP, time zones and TLS |
| [10 — Web and API](10_WEB.md) | UI modes, actual routes, request bodies, consistency and errors |
| [11 — Storage and security](11_STORAGE_SECURITY.md) | Persistence, partitions, update behavior and access boundaries |
| [12 — Tests and delivery](12_TESTS_DELIVERY.md) | Host checks, board procedures, diagnostics and release contents |
| [13 — Limitations](13_DECISIONS_OPEN.md) | Unsupported features and outstanding validation |
| [14 — Validation map](14_TRACEABILITY.md) | Direct links from features to existing tests |

## Shared source material

Source code, API identifiers, Kconfig symbols and license texts are shared between
language versions. See [component sources](../../components/),
[Kconfig](../../main/Kconfig.projbuild), [defaults](../../sdkconfig.defaults),
[partition table](../../partitions.csv), [font records](../../assets/fonts/README.md)
and [license index](../../licenses/README.md). Firmware compilation does not depend
on the documentation language.

The [Polish document index](../00_INDEX.md) provides the unabridged development
archive and ADR references. Update both current-language instructions when behavior
changes, while preserving historical test dates and outcomes.
