# Testy hostowe

Uruchom `python3 -m unittest discover -s tests -v`. Wymagane Python 3.9+
i kompilator C (`cc`), bez instalowania bibliotek Pythona. Testy są także
wykrywane przez pytest, jeśli jest dostępny.

Walidator GPIO: 5 testów, w tym 2332 konfiguracje porównujące wykonany kod
C firmware z walidatorem Pythona; pin RESET, wszystkie sygnały, zakresy,
kolizje, rezerwacje USB/UART i błędne dane. Biblioteka testowa powstaje
wyłącznie w katalogu tymczasowym.

`python3 tools/check_config.py sdkconfig` sprawdza lokalną konfigurację.
CMake uruchamia tę kontrolę automatycznie. Testy hostowe nie zastępują
`idf.py build` ani protokołu testów PCB.

Profil platformy: dodatkowe 3 testy (poprawne defaults, odrzucenie każdej
z 8 błędnych opcji i brakujących opcji). Łącznie 8 testów.
Kontrola pełnego profilu: `python3 tools/check_config.py sdkconfig --platform`.
Telemetryczne zadania i pomiary elektryczne wymagają PCB — patrz
`docs/HARDWARE_BRINGUP.md`; nie są symulowane testem hostowym.

SSD1322: 9 dodatkowych testów wykonujących firmware C na hoście. Obejmują
D/C i inicjalizację, każdą awarię init/present, ujemny błąd transportu,
adresowanie, odrzucenie błędnego profilu/bufora, enable/disable oraz
wzór 4bpp z canary przed i za pamięcią. Łącznie 17 testów.
Nie są testem fizycznego SPI ani panelu. Podgląd: `python3
tools/preview_ssd1322.py build/ssd1322-smoke.png`.

Canvas: 9 nowych testów; łącznie 26 testów hostowych. Oprócz pełnego
obrazu referencyjnego obejmują niezależną wyrocznię dla 600 prostokątów,
clipping widżetów, skrajne współrzędne, maski, nibbles i padding.

Test pamięci renderera (ASan i UBSan, 10000 sekwencji):

```sh
cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -fno-omit-frame-pointer -g -Icomponents/graphics/include -Imain \
  components/graphics/streamoo_canvas.c main/canvas_smoke.c \
  tests/canvas_sanitizer.c -o /tmp/streamoo-canvas-sanitizer
/tmp/streamoo-canvas-sanitizer
```

W środowisku blokującym LeakSanitizer przez ptrace można wykonać pozostałe
kontrole przez `ASAN_OPTIONS=detect_leaks=0 /tmp/streamoo-canvas-sanitizer`.
Taki wynik nie obejmuje wykrywania wycieków; sam renderer nie alokuje pamięci.

Tekst: 6 kolejnych testów, łącznie 32. Walidacja Unicode/UTF-8, długości,
bit7 glifu, clippingu i fallbacku z oryginalnym syntetycznym providerem.
Testy nie dołączają bitmap z ZIP i nie potwierdzają fontu na OLED.

Font PL: 4 nowe testy, łącznie 36. Obejmują wszystkie 113 bitmap przez digest źródła, mapping, ogonki i pełną scenę tekstową. Zobacz `THIRD_PARTY_NOTICES.md` dla statusu importowanych bitmap.

Layout: 6 nowych testów, łącznie 42. Fazy i granice marquee, wyrównanie, długi offset, reset/pominięte klatki i izolacja wierszy.

Font 4bpp: 9 nowych testów, łącznie 51. Spany i pokrycie 113 glifów,
kompozycja alpha, baseline 16px, fallback i błędne metryki, golden frame;
generator: nieparzyste wiersze, skalary i brak glifu w cmap. Zależności hostowe
generatora są opisane w tools/font_generator/README.md; bez nich
kontrole generatora mogą być oznaczone skip (w bieżącym uruchomieniu nie było skipów).

ASan/UBSan fontu (bez niedostępnego w środowisku LeakSanitizera):

```sh
cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined   -fno-omit-frame-pointer -g -Icomponents/graphics/include -Icomponents/text/include   components/graphics/streamoo_canvas.c components/text/streamoo_text.c   components/text/streamoo_text_layout.c components/text/streamoo_font5x8.c components/text/streamoo_bitmap_font.c   components/text/streamoo_ui_font.c tests/bitmap_font_sanitizer.c   -o /tmp/streamoo-bitmap-font-sanitizer
ASAN_OPTIONS=detect_leaks=0 /tmp/streamoo-bitmap-font-sanitizer
```

Scroll tylko w lewo: 4 nowe testy, łącznie 55. Monotoniczny offset, następna kopia wchodząca z prawej (oba fonty), wrap bez cofania, granice uint64. Sanitizer obejmuje teraz oba renderery.
