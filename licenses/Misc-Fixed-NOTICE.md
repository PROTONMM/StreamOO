# X11 Misc Fixed Unicode 5x8 — public domain

Source: https://www.cl.cam.ac.uk/~mgk25/ucs-fonts.html
Archive: https://www.cl.cam.ac.uk/~mgk25/download/ucs-fonts.tar.gz
File: `5x8.bdf`, revision 1.32 (2006-01-05), retrieved 2026-11-14.
Unmodified copy: `assets/fonts/misc-fixed/5x8.bdf`.
SHA-256: `a126e263c6ee843e8725fa3ad630879e74d3294493ce7a00f9a9a21ae10fc2f3`.

The source declares:

> Public domain font.  Share and enjoy.

Markus Kuhn's source page states that the contributors to the Unicode
extensions agreed to retain the public-domain status. It also records that
the original ASCII/Latin-1 authors are not documented. This is the upstream
public-domain declaration, not a newly assigned MIT or BSD license.

StreamOO converts only printable ASCII and the 18 Polish letters to five
column bytes per glyph. All eight BDF rows are retained without scaling or
cropping. The conversion and lookup code are original StreamOO code.
No Arduino, X11 runtime or font library is required by the firmware.

The previous user-provided Polish font is no longer used in the current
font table or regenerated framebuffer fixtures (ADR0102). Historical Git
revisions and reference materials are not relicensed by this replacement.
