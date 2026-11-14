# Current Implementation and Validation Status

[English index](00_INDEX.md)

Snapshot date: 2026-11-14. This page summarizes the current implementation;
it is not a translation of the chronological Polish status log.

## Implemented

Native ESP-IDF radio operation, HTTP/HTTPS with supported codecs and playlist/HLS
transport, STA-first Wi-Fi setup, local web configuration, a persistent catalog,
encoder controls, four visualizations, clock/time zone, mDNS and NVS preferences
are implemented. The default profile uses dr_flac, hardware AES/SHA preparation,
TCP receive window 23040/mailbox 18, and disables synthetic tests/detailed diagnostics.

The current web player has a scrollable full catalog, search, previous/play/stop/next
icons, volume while dragging, codec/bitrate/sample-rate information and three theme
choices. The editor remains paginated. Import appends entries. The same StreamOO
wordmark is used on the OLED, web interface and both README files.

Original work is licensed under MIT with retained third-party exceptions. English
operating/build instructions and current technical documentation live in this directory.

Both README files now list codec profiles, CBR/VBR behavior, HTTP/HTTPS support,
FFT and partial Opus SIMD, and hardware AES/SHA. Fixture bitrates and the HLS
variant-selection limit are explicitly distinguished from guaranteed playback
limits. Audio, DSP and network chapters provide the corresponding technical detail.

## Evidence and limits

Validation of this English documentation update ran the native ESP-IDF 6.1 build,
the GPIO/platform validator and 503 host tests successfully, without skips. The
documentation check also verified relative links/anchors, shell/JSON examples,
Kconfig symbols and the 15 documented API method/route pairs against source. These
are automated-workspace checks, not a new board acceptance run. Documentation changes
do not establish new listening, electrical, Android or power-loss evidence.

The user has confirmed normal operation through several hardware iterations,
accepted the smaller Polish font with a readability reservation, and reported the
compact web controls and live volume working well. Remaining release validation
includes broader codec/source coverage, long-running load, adverse network scenarios,
storage power interruption and board-specific memory/display measurements.

Use [known limitations](13_DECISIONS_OPEN.md), [test procedures](12_TESTS_DELIVERY.md)
and [feature/test mapping](14_TRACEABILITY.md) when preparing a release. Do not
infer that every item is closed from a successful build or a single positive log.

The README capability update also passed the native build, N16R8 validator and
503 host tests without skips. The pinned codec archive hash matches the earlier
audit; disassembly of the current ELF confirms vector arithmetic in the Opus
SILK LPC filter and spectrum FFT kernel. Hardware AES/SHA and early preparation
are enabled in the default and generated configurations. These checks do not
establish per-codec maximum bitrates or a new hardware speedup measurement.

## Historical evidence

The full [Polish STATUS](../STATUS.md), [handover log](../RESUME.md) and
[ADR archive](../adr/) retain earlier measurements, decisions and superseded
experiments. They are intentionally outside the translation scope. Current guides
take care to distinguish active behavior from those historical proposals.
