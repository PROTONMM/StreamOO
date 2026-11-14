# Generator fontu 4bpp

Zweryfikowany pipeline StreamOO UI16, pochodzącego z Monaspace Krypton
Regular v1.400. Wersjonowany plik OTF jest niezmieniony, źródło/licencja
w `assets/fonts/README.md` i `licenses/Monaspace-OFL.txt`. Bitmapowy podzbiór
ma własną nazwę StreamOO UI16; nazwy zastrzeżone pozostają w atrybucji źródła.

Zwykły build ESP-IDF korzysta z wygenerowanego nagłówka i nie wymaga Pillow
ani fontTools. Regeneracja wymaga Pillow >=10, fontTools >=4 i FreeType.
Manifest `assets/fonts/streamoo_ui_16.json` zapisuje wersje użyte do obecnego
zasobu: Pillow 10.2.0, fontTools 4.46.0, FreeType 2.13.2. Zmiana rasteryzatora
może zmienić bitmapy; wynik wymaga przeglądu, nie automatycznej podmiany wzorca.

```sh
python3 tools/font_generator/ttf_to_ssd1322.py \
  assets/fonts/MonaspaceKrypton-Regular.otf --size 13 \
  --out components/text/generated/streamoo_ui_16.h --name streamoo_ui_16 \
  --report assets/fonts/streamoo_ui_16.json --preview build/krypton-atlas.png
```

113 znaków: ASCII 32..126 i 18 liter PL. Rozmiar źródłowy 13 px daje metryki
ascent=13, descent=3, więc wszystkie wybrane glify mieszczą się w wierszu 16 px.
Podzbiór zajmuje 4448 B bitmap, dodatkowo metryki glyphów. Generator sprawdza
cmap, odrzuca brakujące glify i przekroczenie zadeklarowanej wysokości.
Brak glifu nie jest cicho zastępowany kwadratem z pliku źródłowego.

Każdy wiersz glifu jest osobno pakowany do `ceil(width/2)` bajtów, starszy
nibble pierwszy. Nieparzysta szerokość pozostawia zerowy nibble paddingu.
Naprawiono dawny szkic pakujący całą płaszczyznę bez granic wierszy.
Offset/bearing/advance są zapisywane jawnie; renderer używa baseline i
komponuje 4-bitową maskę alpha nad istniejącym framebufferem.

Testy: `python3 -m unittest discover -s tests -v`. Regeneracja/pokrycie cmap
wymaga zależności hostowych; testy runtime fontu wykonują skompilowany C
niezależnie od nich. Preview PNG atlasu to narzędzie analizy, a końcowy wzór
4bpp sceny jest w `tests/fixtures/home_connected.4bpp`.

Mały font5x8: `bdf_to_font5x8.py`; przypięte źródło, deklaracja public domain
i polecenie regeneracji w `../../assets/fonts/README.md`. Nie wymaga Pillow.
