# FLAC decoder sources retained for StreamOO comparison builds

Exact origins, versions and per-file SHA-256 hashes are in ORIGINS.json.
The selected upstream source files are unmodified. StreamOO adapters and
build flags are outside these directories.

micro-flac0.3.0: https://github.com/esphome-libs/micro-flac,
commit f9c27d4d0166021db977d7fc37d205b21d81e9f2. Apache-2.0, see
micro-flac/LICENSE and source notices. Decoder-only subset; encoder and Ogg
demuxer excluded. Build enables Xtensa LPC assembly and PSRAM-only allocation;
defines MICRO_FLAC_DISABLE_OGG. Manifest retained for version/license provenance,
not used to fetch a second component or its submodules.

dr_flac0.13.4: https://github.com/mackron/dr_libs,
commit35b5c392cc15456736c440a68dbe073178bbffe0. MIT-0 selected from the
two alternatives at the end of dr_flac/dr_flac.h; full original header kept.
No stdio, Ogg or SIMD in this comparison build. No upstream changes.

Both are compiled only for the selected FLAC backend. See
docs/FLAC_BACKENDS_BRINGUP.md for integration limits and hardware verification.
