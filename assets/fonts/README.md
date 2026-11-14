# Source font and raster asset

Unmodified source: `MonaspaceKrypton-Regular.otf`, from the official release
[Monaspace v1.400](https://github.com/githubnext/monaspace/releases/tag/v1.400),
asset `monaspace-static-v1.400.zip`, entry
`Static Fonts/Monaspace Krypton/MonaspaceKrypton-Regular.otf`.

Source SHA-256:
`eb8fcc2eb3fef3a8c1f5428e52379fc9badede6eb1f08c4f6d152fc496965e92`.
Copyright (c) 2023 GitHub. SIL OFL-1.1 and reserved font names are retained in
`../../licenses/Monaspace-OFL.txt` from the same release tag.

The converted subset is named **StreamOO UI16**, with Krypton named only as
its source/attribution. Bitmap data remain under OFL-1.1. No modification was
made to the source OTF. Only compiled glyph arrays are linked into firmware;
the OTF and manifest are host/build source assets, not loaded on the device.

`streamoo_ui_16.json` records glyphs, source hash and generation versions.
Regeneration and format: `../../tools/font_generator/README.md`.

## Small font: Misc Fixed Unicode 5x8

`misc-fixed/5x8.bdf` is an unmodified source from Markus Kuhn's X11 Unicode
font archive, revision1.32 (2006-01-05), retrieved2026-11-14. SHA-256:
`a126e263c6ee843e8725fa3ad630879e74d3294493ce7a00f9a9a21ae10fc2f3`.
Public-domain declaration and source URLs: `../../licenses/Misc-Fixed-NOTICE.md`.

The generated `components/text/streamoo_font5x8.c` contains95 printable ASCII
and18 Polish glyphs. BDF MSB-first rows are transposed to bit0-top columns;
all8 rows fit the existing5x8 glyph and6x8 cell. No runtime font dependency,
allocation or change to text layout is introduced. Regenerate offline:

```sh
python3 tools/font_generator/bdf_to_font5x8.py assets/fonts/misc-fixed/5x8.bdf components/text/streamoo_font5x8.c
```

The converter verifies the source hash, required coverage and geometry.
It replaces `tools/import_polish_font.py`; the old private ZIP is no longer
the source of firmware or golden-frame bitmap data. See ADR0102.
