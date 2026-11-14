# Framebuffer reference

`canvas_smoke.4bpp`: 256×64, stride 128 B, row-major, high nibble first,
8192 B. Reference for the diagnostic geometry in `main/canvas_smoke.c`:
rows 16/8/8/32, lower columns 44/132/80, clipped horizontal lines,
diagonals and a 1bpp arrow. Generated once from original project code and
visually reviewed. No fonts, external artwork or device data.

`test_canvas.py` compares the entire result with this frozen file. Updating
it requires reviewing the intended visual change; tests do not overwrite it.

`polish_text.4bpp`: 256×64, stride 128 B, scena `main/text_smoke.c`. Zawiera public-domain Misc Fixed Unicode5x8; źródło i deklaracja są opisane w `licenses/Misc-Fixed-NOTICE.md`. Wzorzec obejrzano jako PNG przed zapisaniem.



`station_list.4bpp`: początkowa scena `ui_list_probe`, 256×64, wzorzec obejrzany na hoście. Zawiera fonty StreamOO UI16 i mały PL opisane w THIRD_PARTY_NOTICES.md; katalog i nazwy są wyłącznie testowe.

`home_connected.4bpp`, `home_waiting.4bpp`, `home_updated.4bpp` i
`home_offline.4bpp`: cztery stany aplikacyjnego `home_probe`, 256×64.
Wszystkie obejrzano jako PNG przed zamrożeniem. Dane są jawnie syntetyczne;
fonty i ich licencje opisuje THIRD_PARTY_NOTICES.md. Zastępują dawną scenę
tekstu/widżetów usuniętą w 04i; test nie odtwarza sam wzorców.
Od ADR0071 korpus widma ma jasność 8, a bieżące szczyty jasność 15; klatki
odświeżono po obejrzeniu tej celowej zmiany kontrastu.

ADR0102 (2026-11-14): wszystkie sześć klatek zawierających mały font
zregenerowano po zastąpieniu jego bitmap przez Misc Fixed Unicode5x8
(public domain). Obejrzano PNG tekstu PL, listy i czterech stanów HOME.
Geometria6x8, układ ekranu i duży font StreamOO UI16 pozostają bez zmian.

ADR0105: klatki HOME odświeżono i obejrzano po celowej zmianie widma na
szachownicę1x1 GS10/czerń. Pozostałe elementy UI bez zmian. Syntetyczny
podgląd obu widm wykonano na tych samych słupkach z osobnymi znacznikami.
