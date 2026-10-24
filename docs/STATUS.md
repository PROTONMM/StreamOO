# Status implementacji

| Etap | Stan | Dowód |
|---|---|---|
| 01 zakres | wymagania zebrane | dokumentacja |
| 02 schemat i profil | walidator i menuconfig zaimplementowane; HARDWARE_TEST_PENDING | ADR 0001; 5 testów hostowych, 2332 konfiguracje; build PASS |
| 03 szkielet | baza zaimplementowana; odbiór sprzętowy i integracyjny OPEN | ESP-IDF v6.1 build PASS; ADR 0002 |
| 04 UI/SSD1322 | OLED działa według użytkownika; 04b canvas gotowy; pełne UI niezaimplementowane | ADR 0003/0004; 26 testów PASS; build PASS; pomiary i nowy obraz pending |
| 05 encoder | NIEZAIMPLEMENTOWANE | — |
| 06 audio | NIEZAIMPLEMENTOWANE | — |
| 07 DSP | NIEZAIMPLEMENTOWANE | — |
| 08 stacje | NIEZAIMPLEMENTOWANE | — |
| 09 sieć | NIEZAIMPLEMENTOWANE | — |
| 10 WWW | NIEZAIMPLEMENTOWANE | — |
| 11 storage/OTA | NIEZAIMPLEMENTOWANE | — |
| 12 testy końcowe | NIEURUCHOMIONE | — |

Aktualizuj tabelę dopiero po wykonaniu i zapisaniu wyniku testów.

## Etap 02 — profil i walidacja GPIO (2026-10-24)

- Komponenty: `board_config`, Kconfig aplikacji, walidator hostowy/CMake.
- Funkcje: pełna mapa wraz z RESET, kolizje, nieistniejące GPIO, pamięć,
  boot straps, USB i konsola UART; host SPI2/SPI3 w menuconfig.
- Testy: `python3 -m unittest discover -s tests -v` — PASS (5 testów,
  2332 porównania konfiguracji C/Python); `python3 tools/check_config.py
  sdkconfig` — PASS; `git diff --check` — PASS.
- `idf.py reconfigure build` — PASS, ESP-IDF v6.1, esp32s3;
  obraz 181344 B (`0x2c460`).
- Sprzęt: HARDWARE_TEST_PENDING — zgodność sygnałów na PCB, zasilanie/reset
  SSD1322 i podciąganie enkodera. Nie wykonano flash/erase/eFuse.
- Ograniczenie: brak automatycznego wykrywania zewnętrznego JTAG;
  konserwatywne ograniczenie pinów strapujących — ADR 0001.
- Commit etapu: `313f542f3e41e53c48f48214fc84040bd41344fd`.
- Następny krok: etap 03 — przypięcie platformy, metryki i zadanie diagnostyczne.

Środowisko: użyto aktywatora EIM
`source /home/mariuszk/.espressif/tools/activate_idf_v6.1.sh` i
`export PATH=/usr/bin:$PATH` (CMake systemowy zamiast Snapa).
`pytest` nie jest zainstalowany; próba instalacji do `/tmp` nie powiodła się
z powodu braku DNS. Testy wykonano przez standardowy `unittest`.

## Etap 03 — baza platformy i eksperyment wejść (2026-10-24)

- Komponenty: `platform`, `hardware_probe`, aplikacja, CMake i walidator profilu.
- Funkcje: kontrola ESP-IDF 6.1/esp32s3, profilu 16 MB/Octal PSRAM 80 MHz/
  CPU 240 MHz; kontrola pamięci/modelu przy starcie; zadanie health co 10 s.
  Opcjonalny raw input probe próbuje A/B/BTN co tick, raportuje co sekundę.
- Testy hostowe: `python3 -m unittest discover -s tests -v` — PASS, 8 testów
  (w tym 2332 konfiguracje GPIO i walidacja 8 opcji platformy).
  `python3 tools/check_config.py sdkconfig --platform` — PASS;
  `git diff --check` — PASS.
- Kompilacja domyślnego profilu: `idf.py reconfigure build` — PASS,
  obraz 182432 B (`0x2c8a0`). Wynik wariantu probe i końcowego buildu poniżej.
- Poprzedni boot i test PSRAM: potwierdzone przez użytkownika przed pracami;
  nie są dowodem uruchomienia nowych zadań.
- HARDWARE_TEST_PENDING: kontrola startowa na PCB, 30 s metryk heap/stosu,
  poziomy wejść i podciąganie enkodera. Brak pomiarów latencji/audio/sieci;
  są zależne od przyszłych komponentów, nie oznaczono ich jako zaliczone.
- ADR: `adr/0002-platform-and-bringup-gate.md`.
- Commit: `efc7e0361b84956a5df0c872f548d56c69baed53`.
- Pełny etap 03 pozostaje częściowy. Nie zaimplementowano stanów radia,
  pipeline audio ani scenariusza zaniku Wi-Fi.
- Następny zakres: 04a — SPI SSD1322 i test grayscale po identyfikacji
  modułu i potwierdzeniu resetu/zasilania. OPEN D17 blokuje wiarygodny wybór
  profilu panelu; D14 blokuje docelową małą czcionkę. Nie zastąpiono tych
  danych przypadkowymi parametrami.

Instrukcja dla użytkownika: `HARDWARE_BRINGUP.md`. Sterownik SSD1322,
pełny UI i dekoder gestów nie zostały jeszcze zaimplementowane.

### Dodatkowa walidacja etapu 03

- Build z `CONFIG_STREAMOO_INPUT_PROBE=y` — PASS, obraz 188288 B
  (`0x2df80`); użyto kopii sdkconfig w `/tmp`, bez zmiany wersjonowanych
  defaults i bez włączania diagnostyki w lokalnym sdkconfig użytkownika.
- CMake zgłasza ostrzeżenia o prywatnych include między `esp_wifi` i
  `wpa_supplicant` wewnątrz ESP-IDF v6.1. Nie pochodzą z nowych komponentów;
  nie wyłączono ostrzeżeń ani mechanizmów walidacji.
- Końcowe `idf.py -DSDKCONFIG=/home/mariuszk/YR/StreamOO/sdkconfig reconfigure build`
  — PASS, ponownie obraz 182432 B; build korzysta z oryginalnego lokalnego
  sdkconfig, probe wyłączony. Nie wykonano flashowania.

## Etap 04a — SSD1322 SPI i test obrazu (2026-10-24)

- Dane: użytkownik zidentyfikował moduł msalamon OLED_312_YELLOW i potwierdził
  R5/R8. Ustalono BS1=0/BS0=0 = 4-wire SPI z oznaczeń PCB i dokumentacji
  kontrolera; sprzeczne podpisy poradnika nie są podstawą do przelutowania.
- Komponenty: `ssd1322` (protokół C + adapter SPI ESP-IDF), zadanie aplikacji
  `display_smoke`, Kconfig, profile, testy hostowe, narzędzie podglądu PNG.
- Funkcje: init z panelem OFF, pełna klatka 4bpp/8192 B, enable/disable,
  sprzątanie zasobów, transport DMA w RAM wewnętrznym, wzór w PSRAM,
  sekwencja diagnostyczna około 12 s i pomiar czasu present. Renderera,
  fontów, stacji i funkcji audio nie implementowano w tym podetapie.
- Profil: dotychczasowe GPIO i 10 MHz zachowane; dopuszczalny zegar SPI
  ograniczony do 10 MHz według tcycle kontrolera. Offset=28/remap=20 są
  propozycją do akceptacji na OLED, nie wynikiem testu sprzętowego.
- `python3 -m unittest discover -s tests -v` — PASS (17 testów).
  Dziewięć nowych obejmuje init/D-C, adresowanie, błędy każdego zapisu,
  bufory i granice pamięci 4bpp oraz zachowanie błędu ujemnego.
- `python3 tools/preview_ssd1322.py build/ssd1322-smoke.png` — PASS;
  podgląd wzoru obejrzany, zgodny z oczekiwanym układem pasów i znaczników.
- `idf.py reconfigure build` — PASS, ESP-IDF v6.1, esp32s3, smoke=y,
  obraz `0x387d0` / 231376 B, 78% bieżącej partycji aplikacji wolne.
  `git diff --check` — PASS. Pozostają znane ostrzeżenia CMake wewnątrz IDF.
- Zmieniono tylko generowane opcje nowego testu w lokalnym sdkconfig poprzez
  reconfigure; nie wersjonowano tego pliku. Nie zmieniono pinów ani profilu
  flash/PSRAM/CPU i nie wykonano flashowania/erase/eFuse.
- HARDWARE_TEST_PENDING: zimny start/reset, obraz i oba brzegi panelu,
  orientacja, użyteczność skali szarości, faktyczne czasy transferu oraz
  pomiary sygnałów/zasilania. ESP_OK w write-only SPI nie jest ACK z panelu.
- ADR: `adr/0003-ssd1322-msalamon-spi-bringup.md`. Instrukcja:
  `HARDWARE_BRINGUP.md`, sekcja odbioru SSD1322.
- Commit: `2b7d0b4c2eb4e1051930b31caa204753b9793ef6`.
- Następny zakres: po odbiorze obrazu 04b — niezależny renderer 4bpp,
  clipping i testy referencyjne. Pełny etap 04 wymaga również rozwiązania
  D14 (konkretny zasób czcionki 5×7 PL), fontu Krypton i UI/widgetów.
  Odbioru sprzętowego 04a nie pominięto i nie oznaczono jako zaliczony.

## Potwierdzenie użytkownika — 2026-10-24

Użytkownik zgłosił: „Wgrałem firmware na płytkę, wyświetlacz działa”.
Dowód: raport użytkownika w rozmowie, po przekazaniu commita 04a
`2b7d0b4`. Nie otrzymano osobno hasha wgranego obrazu ani logów.
Zaliczono podstawowe uruchomienie OLED na jego płytce, co odblokowuje
pracę nad rendererem. Nie dopisano nieprzekazanych wyników: orientacja,
obie krawędzie, pełna skala szarości, zimny start i konkretne czasy SPI
pozostają do uzupełnienia protokołem. Enkoder nie został tym potwierdzony.

## Etap 04b — niezależny canvas 4bpp (2026-10-24)

- Komponenty: `graphics`, aplikacyjna scena `canvas_smoke`, istniejące zadanie
  `display_smoke`, testy i wzorzec klatki. Brak nowych bibliotek zewnętrznych.
- Funkcje: piksel, fill/outline, linie integer, maski transparentne 1bpp,
  clip i canvas potomny. Brak zależności od SSD1322/RTOS/DSP, brak alokacji;
  bufor i jego własność pozostają po stronie aplikacji.
- Testy: `python3 -m unittest discover -s tests -v` — PASS (26 testów).
  9 nowych testów, w tym 600 konfiguracji prostokątów względem niezależnej
  wyroczni, pełne porównanie golden frame, canary i skrajne współrzędne.
- ASan/UBSan: PASS, 10000 sekwencji `tests/canvas_sanitizer.c`.
  LeakSanitizer nie działa w środowisku ptrace; użyto detect_leaks=0,
  więc nie deklarujemy testu wycieków. Renderer nie wykonuje malloc/free.
- `idf.py reconfigure build` — PASS, ESP-IDF v6.1/esp32s3,
  obraz `0x38f50` / 233296 B (końcowe `idf.py build`). `git diff --check` — PASS.
- Do testu OLED dodano piąty obraz: geometria 16/8/8/32, clipping i maska,
  osobny pomiar czasu rasteryzacji. Cała diagnostyka trwa teraz około 15 s,
  następnie wyłącza ekran. Wzór obejrzano na hoście; nie jest docelowym UI.
- HARDWARE_TEST_PENDING: nowa scena canvas i czas rasteryzacji na ESP32-S3;
  nie przypisujemy jej wcześniejszego potwierdzenia użytkownika.
- ADR: `adr/0004-independent-4bpp-canvas.md`.
- Commit: `63a9d5176cb326a76ba44ed66bc93c077f7c6823`.
- Następny zakres 04c: fonty, tekst UTF-8 PL i marquee. D14 nadal OPEN:
  wymagany konkretny zasób małej bitmapowej czcionki 5×7 PL; wysłano pytanie
  o źródło. Nie zastąpiono go przypadkowym fontem. Krypton, teksty i właściwe
  widgety/UI nadal niezaimplementowane; pełnego etapu 04 nie zakończono.

## Etap 04c-1 — źródło fontu i baza UTF-8 (2026-10-24)

- Użytkownik dostarczył ZIP z `glcdfont.c`. Zweryfikowano 256 glifów 5×8
  oraz indeksy 18 polskich liter. D14: źródło ustalone, licencja modyfikacji
  OPEN — ZIP nie zawiera noty licencyjnej/autora; zapytano o te dane.
- `tools/inspect_polish_font.py` tworzy raport i opcjonalny podgląd PNG
  poza źródłami firmware. Podgląd wszystkich 18 liter obejrzany na hoście.
- Komponent `text`: oryginalny dekoder UTF-8, pomiar szerokości i rysowanie
  wiersza 5×8 przez provider glifów, z clippingiem i bez alokacji. Nie
  skopiowano `utf8Rus.cpp`, kodu Arduino ani tablicy bitmap z archiwum.
- `python3 -m unittest discover -s tests -v` — PASS, 32 testy (6 nowych:
  Unicode PL i >1000 skalarów, błędne UTF-8, szerokość, ósmy wiersz glifu,
  clipping i zachowanie znaków zastępczych). Glify testowe są syntetyczne.
- `idf.py build` — PASS, ESP-IDF v6.1/esp32s3. Komponent text skompilowany;
  obraz aplikacji nadal `0x38f50` / 233296 B, ponieważ backend tekstu nie
  jest jeszcze używany w aplikacji. Zachowano test OLED 04b.
- `git diff --check` — PASS. Nie wykonano flash/erase/eFuse.
- ADR: `adr/0005-font-reference-and-utf8.md`.
- Commit: `3b85ffc6fd01939ce28a7818ed6e539535e636f2`.
- Następny krok: wyjaśnienie licencji konkretnej modyfikacji, import glifów
  z notami, test polskich napisów na OLED, następnie marquee/Krypton.
  Polski font nie jest jeszcze w firmware; brak tekstowego testu sprzętowego
  pozostaje HARDWARE_TEST_PENDING. Pełny etap 04 nadal nieukończony.

## Etap 04c-2 — polski font w firmware (2026-10-24)

- Użytkownik wyraźnie polecił skopiować font i pozostawić sprawdzenie
  licencji jako otwarte. Decyzja dotyczy tego zasobu i została wykonana;
  licencja polskich modyfikacji nie została uznana za wyjaśnioną.
- Zaimportowano 95 glifów ASCII i 18 PL (565 B danych bitmapowych), natywny
  lookup Unicode w `text`, bez kodu Arduino i bez konwertera yoRadio.
- Pochodzenie i stan licencji: `THIRD_PARTY_NOTICES.md`; zachowano notę BSD
  bazowego Adafruit GFX. Nie przypisano tej licencji nieznanym zmianom.
- Narzędzie odtwarzania: `tools/import_polish_font.py`; pin SHA-256 źródła
  odrzuca przypadkowe użycie innej wersji pliku.
- Szósty obraz diagnostyczny (`step=5`) pokazuje polskie napisy i ogonki.
  Diagnostyka trwa teraz około 18 s, następnie panel OFF. GPIO bez zmian.
- `python3 -m unittest discover -s tests -v` — PASS (36 testów, 4 nowe).
  Digest 113 bitmap, indeksy Unicode, bit7, klatka referencyjna i canary.
  Podgląd `build/polish-text-smoke.png` obejrzany; wzorzec zamrożony.
- `idf.py reconfigure build` — PASS, ESP-IDF v6.1/esp32s3,
  obraz `0x39910` / 235792 B. `git diff --check` — PASS.
- HARDWARE_TEST_PENDING: czytelność wszystkich 18 liter na OLED, ogonki
  w wierszu 8px i brak nakładania między wierszami. Nie wykonano flash.
- ADR: `adr/0006-provisional-polish-font-import.md`.
- Commit: `git log -1 --format=%H --grep='^stage-04c-2:'` wskazuje commit
  tego podetapu; hash nie może być częścią własnej zawartości.
- Następny zakres: marquee, wyrównanie i font Krypton; licencja małej
  modyfikacji wciąż DO SPRAWDZENIA. Pełnego etapu 04 nie ukończono.

## Odbiór użytkownika po etapie 04c-2 (2026-10-24)

Użytkownik potwierdził: „skompilowałem, wgrałem, działa”, po przekazaniu
commita `867714d` z polskim fontem. Odnotowano udaną kompilację, wgranie
przez użytkownika i podstawowe działanie tej wersji na płytce.
Nie otrzymano oddzielnego hasha obrazu, zdjęcia ani logu; przypisanie do
04c-2 wynika z kontekstu rozmowy. Szczegółowa ocena wszystkich 18 glifów,
ogonków i odstępów oraz pomiary czasów nie zostały osobno przekazane.
Pozostają punktami do uzupełnienia protokołu, nie blokują dalszej pracy
nad marquee i wyrównaniem. Status licencji zmian PL pozostaje otwarty.
Zmiana dotyczy wyłącznie dokumentacji; bez zmian firmware i ponownego buildu.

## Etap 04d — wyrównanie i marquee (2026-10-24)

- `text`: wyrównanie left/center/right, clipping wiersza i czysty zegarowy
  marquee ping-pong. Rozszerzona pozycja bez zawijania int16 dla długich tekstów.
- Aplikacja: po 18 s statycznych obrazów dodano 8 s animacji, razem około
  26 s do wyłączenia panelu. Proponowane 30 px/s i pauzy 1 s; bez realnych
  metadanych. Nagłówek nadal ma mały font; Krypton jest kolejnym podetapem.
- Testy: 42 PASS (`python3 -m unittest discover -s tests -q`), w tym 6 nowych.
  `idf.py reconfigure build` — PASS. `git diff --check` — PASS.
- HARDWARE_TEST_PENDING: czytelność przewijania, pauzy, wyrównanie, pomiary
  liczby klatek/czasu na ESP32-S3. Nie wykonano flash/erase/eFuse.
- ADR 0007. Następnie: font Krypton i renderer glifów 4bpp.
- Metryki każdego nowego commita: trailery AI-Model i AI-Tokens, według
  `COMMIT_METRICS.md`. Rodzina modelu GPT-6/Codex, wariant runtime oraz
  licznik tokenów nie są udostępnione; brakujące dane oznaczono unavailable.
- Hash: `git log -1 --format=%H --grep='^stage-04d:'`.
