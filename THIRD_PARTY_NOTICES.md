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
`tests/fixtures/krypton_layout.4bpp`. Source hash, release URL and exact
regeneration process are documented in `assets/fonts/README.md`.

The station-list reference `tests/fixtures/station_list.4bpp` contains the same font shapes described above: StreamOO UI16 (OFL-1.1) and the user-authorized small PL font with modification license still under review.
