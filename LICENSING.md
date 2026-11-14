# Licensing scope

Copyright (c) 2026 Mariusz Księżak.

Original StreamOO code and documentation are licensed under the standard
[MIT License](LICENSE), SPDX identifier `MIT`. This includes original firmware
modules and adapters, the web interface, tools, tests, configuration, and
original artwork/audio fixtures, except for the third-party material below.
This scope statement identifies covered material; it adds no restrictions
to the MIT license text.

## Third-party material

The root MIT license does not relicense third-party code, fonts, data, or
reference material. Existing copyright notices, license texts and per-file
terms remain in force. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
for versions, origins and attributions and [licenses/README.md](licenses/README.md)
for the retained license texts.

| Material | Applicable terms |
| --- | --- |
| `components/decoder/third_party/dr_flac/` | MIT-0, the selected upstream alternative; original header retained |
| `components/decoder/third_party/micro-flac/` | Apache-2.0; original LICENSE and provenance retained |
| `assets/fonts/MonaspaceKrypton-Regular.otf`, derived glyph data in `components/text/generated/streamoo_ui_16.h` and font manifests | SIL OFL-1.1; font data retain their terms after conversion |
| `assets/fonts/misc-fixed/5x8.bdf` and derived small-font bitmap data in `components/text/streamoo_font5x8.c` | Upstream public-domain declaration; original converter/lookup code is MIT |
| Font-bearing framebuffer/image fixtures, including `tests/fixtures/` and boot previews | Original composition/rendering is MIT; included font material retains OFL-1.1 or public-domain terms, as applicable |
| `licenses/`, including `licenses/sources/cacrt_all.pem` | Retained upstream terms; Mozilla CA data are MPL-2.0 |
| `references/` and source-project/hardware materials | Excluded from the root MIT grant; original rights and permissions apply. Unresolved rights are not replaced by MIT |
| ESP-IDF, managed components, toolchain runtime, linked vendor archives | Their own terms, including Apache, BSD, MIT, MPL and the GCC Runtime Library Exception; not relicensed by this project |

Original MIT wrapper code and third-party data may coexist in a generated
file. The data's upstream terms remain applicable; the original wrapper does
not change them. Historical Git revisions containing replaced third-party
material retain their original licensing status.

## Espressif audio codecs

The current firmware links `esp_audio_codec` 2.6.2 under
[Espressif Modified MIT](licenses/Espressif-Audio-Codec-Modified-MIT.txt).
That component is restricted to use with Espressif products, including in
binary form. Choosing dr_flac for FLAC does not remove the other Espressif
codec/parser libraries. The original StreamOO application is MIT, but the
combined firmware must also comply with these vendor terms.

## Redistributing sources or firmware

Include `LICENSE`, this scope statement, `THIRD_PARTY_NOTICES.md`, applicable
upstream license/NOTICE files and required corresponding source data with
the distribution. Preserve the notices in vendored source files. The retained
Mozilla CA source is provided in `licenses/sources/cacrt_all.pem` under MPL-2.0.

For a binary release, retain the notices for the actual ESP-IDF, managed
components and runtime versions used to build that image. The checked-in
license collection supplements those packages; it is not a claim that every
possible configuration or future SDK version has been audited. The existing
[component inventory](docs/MIT_MODULE_INVENTORY.csv) records an inspected build,
not a universal inventory of all build options.

Do not describe the combined image as containing only MIT-licensed material.
The root MIT grant does not itself grant rights to redistribute reference
materials with unresolved provenance. Private configuration and playlists
are not part of the licensed distribution.
