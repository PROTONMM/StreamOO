# Testy hostowe

Aktualizacje OLED po zmianie pikseli: testy w test_ssd1322.py oraz stress
GDDRAM/awarii częściowego zapisu:

```sh
cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -g -Icomponents/ssd1322/include components/ssd1322/ssd1322_protocol.c tests/ssd1322_update_sanitizer.c -o /tmp/streamoo-ssd1322-update-sanitizer
ASAN_OPTIONS=detect_leaks=0 /tmp/streamoo-ssd1322-update-sanitizer
```

LeakSanitizer wyłączony ze względu na ptrace środowiska; test nie alokuje.

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

Font PL (ADR0102): Misc Fixed Unicode5x8, public domain. Testy obejmują113 bitmap przez zamrożony digest, niezależne porównanie pikseli z BDF, odtwarzalną regenerację C, mapping Unicode, ogonki, granice bufora i pełną scenę tekstową. Źródło/deklaracja: `licenses/Misc-Fixed-NOTICE.md`.

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

Wizualizacje: 7 nowych testów, łącznie 62. Izolacja bounds/clip, brak mutacji
modelu, oba kanały i peak, empty/full spectrum, needle endpoints, invalid
snapshots oraz małe/nieparzyste bounds.

```sh
cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined   -fno-omit-frame-pointer -g -Icomponents/graphics/include -Icomponents/ui_widgets/include   components/graphics/streamoo_canvas.c components/ui_widgets/streamoo_visualization.c   tests/visualization_sanitizer.c -o /tmp/streamoo-visualization-sanitizer
ASAN_OPTIONS=detect_leaks=0 /tmp/streamoo-visualization-sanitizer
```

LeakSanitizer jest niedostępny w obecnym środowisku ptrace; powyższy wynik
nie jest testem wycieków. Komponent nie wykonuje alokacji.

UI/lista: 11 nowych testów, łącznie 73. Rozdzielenie browse/cancel od komend,
ID+generation, kolejka pełna/retry, retarget, granice, snapshot replacement,
pusta lista, clipping, jasność i pełny automat diagnostyczny.

```sh
cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined   -fno-omit-frame-pointer -g -Icomponents/graphics/include -Icomponents/text/include   -Icomponents/ui/include -Icomponents/ui_widgets/include -Imain components/graphics/streamoo_canvas.c   components/text/streamoo_text.c components/text/streamoo_text_layout.c   components/text/streamoo_font5x8.c components/text/streamoo_bitmap_font.c   components/text/streamoo_ui_font.c components/ui/streamoo_ui.c   components/ui/streamoo_ui_list.c components/ui/streamoo_ui_home.c components/ui_widgets/streamoo_visualization.c main/ui_list_probe.c tests/ui_sanitizer.c   -o /tmp/streamoo-ui-sanitizer
ASAN_OPTIONS=detect_leaks=0 /tmp/streamoo-ui-sanitizer
```

Jak wcześniej, niedostępny LeakSanitizer nie jest częścią wyniku; wykonano
ASan/UBSan oraz canary. UI nie wykonuje alokacji.

HOME 04i: 12 nowych testów, dwie dawne integracje zastąpione nowymi,
łącznie 83. Cztery wzorce pełnego ekranu, stale snapshots/metadane, kalendarz,
clock offline, reset tekstów, clipping, skrajne RSSI/bitrate i widżety.

```sh
cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined   -fno-omit-frame-pointer -g -Icomponents/graphics/include -Icomponents/text/include   -Icomponents/ui_widgets/include -Icomponents/ui/include -Imain   components/graphics/streamoo_canvas.c components/text/streamoo_text.c   components/text/streamoo_text_layout.c components/text/streamoo_font5x8.c   components/text/streamoo_bitmap_font.c components/text/streamoo_ui_font.c   components/ui_widgets/streamoo_visualization.c components/ui/streamoo_ui.c   components/ui/streamoo_ui_list.c components/ui/streamoo_ui_home.c   main/home_probe.c tests/home_sanitizer.c -o /tmp/streamoo-home-sanitizer
ASAN_OPTIONS=detect_leaks=0 /tmp/streamoo-home-sanitizer
```

1000 kolejnych klatek sprawdza fazy HOME, usuwanie starych danych i canary.
LeakSanitizer niedostępny w ptrace, bez deklaracji testu wycieków.

Enkoder 05a: `test_input.py` kompiluje i wykonuje `input_test.c` z ostrzeżeniami
jako błędami. Łącznie 84 testy; nie jest to test szybkości sprzętu GPIO.

Regresje 05b w tym samym harnessie: stałe B nie tworzy detentów,
krótkie sekwencje dostarczone przez capture w tej samej ms dają półkroki.
Transport IRQ/kolejka i overflow wymagają osobnego odbioru na sprzęcie.

Sterowanie 05c: 7 nowych testów, łącznie 91. Harness `controls_test.c`
sprawdza tryby A/B, cykl widżetów bez volume, listę confirm/cancel bez
zmiany playing_id, odrzucone komendy, puste katalogi, skrajne delta
oraz 1000 klatek interaktywnej aplikacji z canary i wyborem stacji.

```sh
STREAMOO_CONTROLS_SANITIZE='-fsanitize=address,undefined -fno-omit-frame-pointer -g' ASAN_OPTIONS=detect_leaks=0 python3 -m unittest discover -s tests -p test_controls.py -q
```

ASan/UBSan PASS; bez deklaracji testu wycieków (LeakSanitizer pod ptrace
wyłączony). Kolejki FreeRTOS i interaktywne sterowanie wymagają OLED/enkodera.

Preferencje 05d: 8 nowych testów (łącznie 99), w tym wszystkie 303
prawidłowe pary pól, walidacja blobu, debounce, powrót do saved, sklejenie
A→B→A, retry po błędzie i uptime ponad 32-bit ms. Adapter flash/FreeRTOS
wymaga sprzętu; testy hostowe obejmują format i politykę.

```sh
STREAMOO_PREFERENCES_SANITIZE='-fsanitize=address,undefined -fno-omit-frame-pointer -g' ASAN_OPTIONS=detect_leaks=0 python3 -m unittest discover -s tests -p test_preferences.py -q
```

ASan/UBSan PASS, LeakSanitizer pod ptrace wyłączony, bez testu wycieków.

Audio06a: 8 nowych testów (łącznie107): cisza i koniec, izolacja L/R,
częstotliwości, poziom/RMS/DC, rampy, chunk invariance, canary, błędne argumenty
i cała sekwencja480000 ramek. Sterownik I2S/DMA i analogowy DAC wymagają sprzętu.

```sh
cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -g -Imain main/audio_tone.c tests/audio_tone_sanitizer.c -o /tmp/streamoo-audio-tone-sanitizer
ASAN_OPTIONS=detect_leaks=0 /tmp/streamoo-audio-tone-sanitizer
```

ASan/UBSan PASS, 2100 bloków z canary, końcowa cisza. LeakSanitizer pod
ptrace wyłączony, bez testu wycieków. ELF/objdump użyto do sprawdzenia
IRAM generatora/callbacków i DRAM tablicy/stanu; nie jest to test hardware.

Audio06b: 14 nowych testów (łącznie121): krzywa/gain0..100, rampy/retarget,
unity dla ekstremów int32, chunk invariance, kanały/canary, czysty właściciel
komend, clamp/revision i zewnętrzny właściciel w UI oraz źródło ciągłe.
Adaptery FreeRTOS/DMA i słyszalne przejścia nadal wymagają sprzętu.

```sh
cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -g -Imain -Icomponents/audio_gain/include -Icomponents/ui/include -Icomponents/ui_widgets/include -Icomponents/graphics/include -Icomponents/input/include components/audio_gain/streamoo_audio_gain.c main/audio_control_model.c main/audio_tone.c tests/gain_sanitizer.c -o /tmp/streamoo-gain-sanitizer
ASAN_OPTIONS=detect_leaks=0 /tmp/streamoo-gain-sanitizer
```

ASan/UBSan gain/source/model oraz 8 testów UI/control PASS; LeakSanitizer pod
ptrace wyłączony, bez testu wycieków. Objdump/ELF: gain i callbacki IRAM,
dane DRAM, bez helperów z flash w gain. Bez pomiaru hardware WCET/jitter.

PCM06c: 11 nowych testów (łącznie132). Pełny/pusty FIFO bez nadpisywania,
wrap liczników/sekwencji, 20000 bloków między dwoma wątkami, tap przed
gain0/50/100, rezerwa/rampa/powrót, STOP konsumujący źródło, przeciążony
tap bez zmiany PCM audio, startup i awaryjny brak danych.

```sh
STREAMOO_PCM_SANITIZE='-fsanitize=address,undefined -fno-omit-frame-pointer -g' ASAN_OPTIONS=detect_leaks=0 python3 -m unittest discover -s tests -p test_pcm.py -q
```

ASan/UBSan PASS; LeakSanitizer pod ptrace wyłączony, bez testu wycieków.
Test dwóch wątków nie jest dowodem wykrywania wszystkich wyścigów; ThreadSanitizer
nie był uruchamiany. Na target sprawdzono natywne load/store z barierami,
brak helperów __atomic, IRAM funkcji i wewnętrzny RAM tablic. Odbiór FreeRTOS,
DMA, słyszalnego rebuffer i przeciążenia tap pozostaje sprzętowy.

MP3/adapter06d: 9 nowych testów, łącznie141. Wszystkie65536 wartości s16,
mono/stereo/endianness/unaligned, błędne formaty i granice, zawijanie pętli
w bloku, hash/168 nagłówków ramek fixture. Nie wykonują vendorowego dekodera
na hoście. Jego działanie/metadane i odsłuch wymagają ESP32-S3.

```sh
cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -g -Icomponents/pcm_adapter/include components/pcm_adapter/streamoo_pcm_adapter.c tests/pcm_adapter_sanitizer.c -o /tmp/streamoo-pcm-adapter-sanitizer
ASAN_OPTIONS=detect_leaks=0 /tmp/streamoo-pcm-adapter-sanitizer
```

ASan/UBSan konwersji i pętli PASS; LeakSanitizer wyłączony pod ptrace, bez
testu wycieków. FFmpeg służy do generowania/referencji własnego klipu,
nie jest linkowany do firmware. Licencje vendor sprawdzone, binaria ESP-only.

ADR0103: czwarta wizualizacja (RMS amplitud prążków pasma), zgodność starych
preferencji0..2 i nowego ID3. Oracle NumPy dla obu FFT1024/2048, porównanie
sc16/float, pojedyncze prążki/szerokie pasma/cisza, expiry obu snapshotów,
przełączanie i clipping czterech trybów. Bez dodatkowej FFT w I2S.

ADR0105 zastępuje spektralne RMS z ADR0103/0104: obie wizualizacje widma
używają maksimum i skali−45..0dBFS/gamma2. Testspectrum_envelope sprawdza
hold500ms, release2000ms, nowe szczyty, niezależne pasma, ciszę, reset
i cofnięty czas; renderer sprawdza identyczne słupki/szachownicę i osobne
znaczniki w trybie3. RMS stereo w bargraphie/wskazówkach pozostaje.
