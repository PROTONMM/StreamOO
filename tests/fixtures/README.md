# Framebuffer reference

`canvas_smoke.4bpp`: 256×64, stride 128 B, row-major, high nibble first,
8192 B. Reference for the diagnostic geometry in `main/canvas_smoke.c`:
rows 16/8/8/32, lower columns 44/132/80, clipped horizontal lines,
diagonals and a 1bpp arrow. Generated once from original project code and
visually reviewed. No fonts, external artwork or device data.

`test_canvas.py` compares the entire result with this frozen file. Updating
it requires reviewing the intended visual change; tests do not overwrite it.

`polish_text.4bpp`: 256×64, stride 128 B, scena `main/text_smoke.c`. Zawiera importowane bitmapy fontu — pochodzenie, polecenie użytkownika i nierozstrzygnięta licencja zmian PL są opisane w `THIRD_PARTY_NOTICES.md`. Wzorzec obejrzano jako PNG przed zapisaniem.
