# Status implementacji

Bieżący punkt wznowienia: [RESUME.md](RESUME.md).

| Etap | Stan | Dowód |
|---|---|---|
| 01 zakres | wymagania zebrane | dokumentacja |
| 02 schemat i profil | walidator i menuconfig zaimplementowane; HARDWARE_TEST_PENDING | ADR 0001; 5 testów hostowych, 2332 konfiguracje; build PASS |
| 03 szkielet | baza zaimplementowana; odbiór sprzętowy i integracyjny OPEN | ESP-IDF v6.1 build PASS; ADR 0002 |
| 04 UI/SSD1322 | HOME i lista odebrane na OLED; końcowe strojenie fontu OPEN | ADR 0012; odbiór użytkownika |
| 05 encoder | dekoder/IRQ i gesty odebrane; interaktywny adapter odebrany w trybie A; preferencje NVS odebrane; shutdown/flush OPEN | ADR 0013–0017; 99 testów PASS |
| 06 audio | I2S/gain/PCM stress odebrane; MP3 decode/PCM potwierdzone logiem; AAC-LC PoC odebrane; AAC incremental file decode06f odebrane logiem i odsłuchem; źródło/bufor AAC06g odebrane logiem, odsłuchem i sterowaniem; niezależny reader06h gotowy, odbiór S3 OPEN; sieć/player OPEN | ADR 0018–0025; 168 testów hostowych PASS; build źródeł PASS |
| 07 DSP | NIEZAIMPLEMENTOWANE | — |
| 08 stacje | NIEZAIMPLEMENTOWANE | — |
| 09 sieć | NIEZAIMPLEMENTOWANE | — |
| 10 WWW | NIEZAIMPLEMENTOWANE | — |
| 11 storage/OTA | NVS volume/widżet odebrane; flush i pozostały zakres OPEN | ADR 0017; odbiór użytkownika |
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
- Hash: `ce9a6ade18a43db55623917794e1c1656809c9c2`.

## Etap 04e — font Krypton i alpha 4bpp (2026-10-24)

- Zaimplementowano renderer glifów 4bpp z baseline/bearing/advance,
  Unicode lookup, fallback, clippingiem i kompozycją alpha nad tłem.
  Brak alokacji i zależności od SSD1322/DSP/RTOS.
- Oficjalny source font Krypton v1.400, OFL z copyright zachowane. Nazwa
  pochodnego podzbioru: StreamOO UI16. Font źródłowy jest niezmieniony.
- 113 glifów (ASCII i 18 PL), 4448 B bitmap plus metryki, rasteryzacja 13px
  daje kompletny wiersz 16px (ascent 13 + descent 3). Naprawiono pakowanie
  nieparzystych wierszy generatora i sprawdzanie cmap/metryk.
- W scenie animacji nagłówek używa większego fontu; małe wiersze nadal
  korzystają z fontu PL i marquee. Całość diagnostyki około 26 s.
- `python3 -m unittest discover -s tests -q` — PASS, 51 testów, bez skipów.
  Testy rendererów i generatora, pełna zamrożona klatka obejrzana na hoście.
- ASan/UBSan `tests/bitmap_font_sanitizer.c` — PASS. LeakSanitizer pominięty
  z powodu ograniczeń ptrace (detect_leaks=0); nie jest to test wycieków.
- `idf.py reconfigure build`, końcowe `idf.py build` — PASS, ESP-IDF 6.1,
  esp32s3, obraz 0x3bf80 / 245632 B. `git diff --check` — PASS.
- HARDWARE_TEST_PENDING: czytelność dużego fontu, akcentów i ogonków,
  separacja wierszy, antyaliasing, centrowanie i ruch tekstu. Nie wykonano flash.
- ADR 0008; OFL i źródło opisane w THIRD_PARTY_NOTICES.md i assets/fonts.
  Otwarta licencja małego fontu PL pozostaje bez zmian.
- Następnie: po potwierdzeniu czytelności fontów/layoutu na OLED — dalszy UI,
  wymienne widgety i lista stacji. Pełnego etapu 04 nadal nie zakończono.
- Hash: `b8ca9569df9ecb75aa2067e2b47dfd7a47b8061a`.
- Model/tokeny: trailery commita, według COMMIT_METRICS.md; nieudostępnione
  dane są oznaczone unavailable, nie oszacowane.

## Odbiór fontu i zmiana scrolla — etap 04f (2026-10-24)

- Użytkownik potwierdził działanie, poprawne polskie znaki i scroll po 04e.
  Czcionka jest tymczasowo akceptowana, wygląd wymaga późniejszej poprawy.
  Nie dopisano nieprzekazanych pomiarów ani docelowej akceptacji typografii.
- Nowe wymaganie: ruch tylko od prawej do lewej. Usunięto aktywny ruch
  powrotny; powtórzenie tekstu wchodzi z prawej po odstępie 24px (propozycja).
  Dotyczy obu fontów. Offset uint64 i clipping chronią granice i długie teksty.
- Scena animowana trwa 20 s, cała diagnostyka około 38 s, potem panel OFF.
  Daje to czas na zobaczenie pełnego cyklu przy aktualnych długościach tekstu.
- `python3 -m unittest discover -s tests -q` — PASS, 55 testów (4 nowe).
  Sprawdzono brak cofania, powtórzenie, oba renderery, granice i bufory.
- ASan/UBSan renderera fontów — PASS; program rozszerzony na font 5×8.
  LeakSanitizer pominięty z powodu ptrace (detect_leaks=0), bez deklaracji
  testu wycieków. `idf.py reconfigure build` i `git diff --check` — PASS.
  Obraz ESP32-S3: 0x3c060 / 245856 B.
- HARDWARE_TEST_PENDING: odbiór nowej pętli lewostronnej na OLED.
  Nie wykonywano flash/erase/eFuse. ADR: 0009.
- Następny zakres: widgety i lista stacji; osobno dalsze strojenie wyglądu
  czcionki. Poprawność PL z poprzedniej wersji jest potwierdzona przez użytkownika.
- Hash: `30df2e2593f616d7862e1923f4a6b79d2f92e9ef`.
- Model i tokeny w trailerach commita według COMMIT_METRICS.md.

## Odbiór 04f i etap 04g — wizualizacje (2026-10-24)

- Użytkownik potwierdził prawidłowe działanie ostatniej wersji, po 30df2e2.
  Zapisano funkcjonalny odbiór pętli scrolla. Dokładnego hasha urządzenia,
  zdjęć i pomiarów nie przekazano. Font nadal tymczasowo akceptowany.
- Nowy komponent `ui_widgets`: spectrum, bargraph L/R z peak i dwa mierniki
  wskazówkowe. Wspólny snapshot i bounds, clipping, bez zależności od
  sterownika/DSP i bez alokacji. Zakresy danych prezentacji są jawne.
- Aplikacyjny `visualization_probe` dostarcza wyłącznie dane syntetyczne,
  oznaczone TEST. Tryb zmienia się co 6 s w dotychczasowej animacji 20 s.
  Nagłówek, polskie znaki i scroll pozostają. Pełny test nadal około 38 s.
- Nie zaimplementowano FFT/Hann/analizy audio ani gestu zmiany trybu.
  Te zadania pozostają w etapach 05/07; nie uznano ich za wykonane przez UI.
- `python3 -m unittest discover -s tests -q` — PASS, 62 testy (7 nowych).
  Granice/clip, kanały, peak, 0..64 pasm, needle endpoints, invalid inputs,
  minimalne i nieparzyste bounds. Model po renderze pozostaje niezmieniony.
- ASan/UBSan `tests/visualization_sanitizer.c` — PASS; LeakSanitizer pominięty
  pod ptrace (detect_leaks=0), bez deklaracji testu wycieków.
- `idf.py reconfigure build` — PASS; obraz ESP32-S3 0x3c700 / 247552 B.
  `git diff --check` — PASS.
- HARDWARE_TEST_PENDING: trzy nowe widżety, czytelność podwójnego miernika,
  pasm i peak oraz brak artefaktów przy przełączeniu. Nie wykonano flash.
- ADR 0010. Następnie: animowana lista i właściciel stanu UI, integracja
  gestów dopiero przy sterowniku wejścia; dane audio dopiero po DSP.
- Hash: `4ce01fdcb54d4aa132f1f248b4d08ce0b72f4e19`.
- Model/tokeny w trailerach zgodnie z COMMIT_METRICS.md; niedostępnych
  liczników nie oszacowano.

## Zasady wiadomości commitów (2026-10-24)

Na nowe polecenie użytkownika wyłączono dodawanie trailerów AI-Model i
AI-Tokens do kolejnych wiadomości commitów. Zaktualizowano AGENTS.md
oraz COMMIT_METRICS.md. Wcześniejsza historia pozostaje bez zmian.
Zmiana wyłącznie dokumentacyjna; `git diff --check` — PASS, bez buildu firmware.

## Potwierdzenie i etap 04h — lista stacji i właściciel UI (2026-10-24)

- Użytkownik potwierdził działanie po wdrożeniu widżetów 04g (według kontekstu
  4ce01fd); szczegółowych pomiarów i hasha urządzenia nie przekazano.
  Wcześniejsza uwaga o późniejszym dopracowaniu czcionki pozostaje aktualna.
- Komponent `ui`: HOME/STATIONS, logiczny wybór i animacja Q16, otwarcie,
  ruch, cancel/confirm. Catalog snapshot z trwałymi ID i generation,
  pojedynczy właściciel UI i nonblocking callback do przyszłej kolejki.
- Browse/cancel bez komendy; confirm wysyła ID+generation dopiero przy
  przyjęciu przez adapter. Pełna kolejka zostawia listę otwartą. Snapshot
  playing_id nie jest zmieniany przez UI. Nie ma jeszcze playera/audio.
- Renderer: wyróżniony środkowy cel, przygaszone sąsiednie wiersze, clipping,
  lewostronny marquee zaznaczonej nazwy i obsługa pustego katalogu.
- Parametry testowe: animacja 160ms, granice bez wrap. D19 nadal OPEN;
  nie przyjęto docelowego zawijania ani timeoutu bez potwierdzenia UX.
- `ui_list_probe`: syntetyczna lista TEST i automatyczne zdarzenia, bez
  sterowania enkoderem. Pokaz 8s po poprzedniej sekwencji, razem około 46s.
  Log requested=1003/snapshot=1002 potwierdza rozdzielenie modelu w diagnozie.
- `python3 -m unittest discover -s tests -q` — PASS, 73 testy (11 nowych).
  Zbadano stany, komendy, queue failure, animację, katalog, clipping, jasność
  oraz scenę/skrypt z referencyjnym framebufferem obejrzanym na hoście.
- ASan/UBSan `tests/ui_sanitizer.c` — PASS: skrypt i 1000 retargetów.
  LeakSanitizer pominięty pod ptrace (detect_leaks=0), bez testu wycieków.
- `idf.py reconfigure build` — PASS; obraz esp32s3 0x3d140 / 250176 B.
  Usunięto wykryty błąd brakującego nagłówka layoutu w scenie aplikacji;
  ponowny build i test integracyjny sceny przeszły. `git diff --check` — PASS.
- HARDWARE_TEST_PENDING: nowa lista na OLED, wyróżnienie celu, animacja
  i przejścia. Nie wykonano flash/erase/eFuse. ADR 0011.
- Dalszy zakres: pełny ekran HOME i integracja gestów po sterowniku wejścia;
  rzeczywiste katalogi/player dopiero w ich etapach. Pełny etap 04 nieukończony.
- Hash: `5aec16d62022a640e243f69d6d08fc50e7237a8e`.

## Odbiór pokazu listy 04h (2026-10-24)

Użytkownik potwierdził: „Działa, wybór stacji jest OK”. Zapisano pozytywny
odbiór listy i wyboru w pokazie 04h na OLED, według kontekstu wersji
5aec16d. Nie otrzymano osobnego hasha obrazu ani logów pomiarowych.
Sterowanie w tej wersji było automatyczne na katalogu TEST; potwierdzenie
nie oznacza obsługi fizycznego enkodera ani odtwarzania rzeczywistych stacji.
Kwestia docelowego wrap-around i dopracowania wyglądu czcionki pozostaje
otwarta. Zmiana tylko dokumentacyjna; `git diff --check` — PASS, bez nowego
buildu firmware.

## Etap 04i — pełny HOME ze snapshotu (2026-10-24)

- Komponent `ui`: pełna kompozycja 16/8/8/32, codec/bitrate i Wi-Fi,
  trzy widżety, data i czas. Wybrany widżet należy do UI, nie do DSP.
- Kopia wartości snapshotu i pożyczone napisy z jawnymi długościami. Brak
  alokacji, I/O, odczytu zegara systemowego i bezpośredniego stanu playera.
- Klucz stacja/generacja i text_revision: starsze snapshoty odrzucone,
  stare metadane/codec/pomiary czyszczone; znikanie metadanych czyści wiersze.
  Sam set_playing też usuwa poprzednie teksty. Zegar i RSSI nie resetują scrolla.
- Brakujące dane mają placeholdery. Zegar cywilny i połączenie są niezależne.
  ISO data/24h oraz progi ikonki RSSI są propozycją; D20 nadal OPEN.
- W miejsce starych scen tekstu/widżetów jest spójny home_probe, dane TEST:
  pełne, nieaktualne, zaktualizowane, offline. 20s HOME + dotychczasowa reszta
  daje nadal około 46s, potem OFF. Nie uruchomiono sieci, SNTP, playera ani DSP.
- Usunięto stare nieużywane sceny i klatkę; nowe 4 wzorce obejrzano na hoście.
- `python3 -m unittest discover -s tests -q` — PASS, 83 testy, bez skipów
  (12 nowych HOME, 2 stare integracje zastąpione nowymi scenami).
- ASan/UBSan HOME i listy — PASS, w tym 1000 klatek HOME i dotychczasowy
  skrypt/retargety listy. LeakSanitizer pominięty pod ptrace (detect_leaks=0);
  nie deklarujemy testu wycieków. `git diff --check` — PASS.
- `idf.py reconfigure build` — PASS, ESP-IDF 6.1/esp32s3, obraz 0x3dde0 /
  253408 B. Nie wykonano flash/erase/eFuse.
- HARDWARE_TEST_PENDING: pełny HOME, clock/Wi-Fi, 3 dolne kolumny,
  czyszczenie starych tekstów i zmiana stanów. Wcześniejszy odbiór listy
  nie jest potwierdzeniem tej nowej kompozycji. ADR 0012.
- Następny zakres: odbiór HOME i fizyczny enkoder/gesty etapu 05.
  Finalne strojenie fontów, format czasu i rzeczywiste źródła danych pozostają.
- Hash: `git log -1 --format=%H --grep='^stage-04i:'`.

## Odbiór HOME 04i (2026-10-24)

Użytkownik potwierdził: „Testy prawidłowe, weszystko działa”. Zapisano
pozytywny odbiór pokazu HOME na OLED według kontekstu wersji `8f895bd`.
Nie otrzymano osobnego hasha wgranego obrazu ani logów pomiarowych.
Odbiór dotyczy syntetycznych scen TEST opisanych w 04i; integracja
rzeczywistej sieci, zegara, playera i DSP pozostaje w dalszych etapach.
Kolejny zakres: fizyczny enkoder i gesty etapu 05. Dopracowanie czcionki
i otwarte decyzje pozostają aktualne. Zmiana tylko dokumentacyjna;
`git diff --check` — PASS, bez ponownego buildu firmware.

## Etap 05a — enkoder: czysty dekoder i diagnostyka (2026-10-24)

- Własny input: legalne przejścia A/B, pół/pełne kroki, odwrócenie,
  debounce 20ms, short/long 600ms przy zwolnieniu, pressed-rotate tłumi klik.
- Adapter GPIO wypisuje gesty TEST; nowy domyślny przełącznik
  STREAMOO_ENCODER_GESTURE_TEST. GPIO/pull-up niezmienione, bez komend UI.
- Ograniczenie jawne: polling co tick, szybkie ruchy mogą ginąć.
  To charakterystyka wolnych ruchów, nie gotowy transport produkcyjny.
- 84 testy unittest PASS (nowy harness C sprawdza kierunki, kroki, bounce,
  nielegalne przejścia, debounce, long, boot-held i obrót z przyciskiem).
- ADR 0013. HARDWARE_TEST_PENDING: kroki/ząbek, kierunek, pull-up, gesty.
- Dalej: przechwytywanie zboczy/overflow, pomiary i mapowanie A/B do UI;
  trwałość NVS i player zgodnie z ich etapami. Cały etap 05 częściowy.
- Build ESP-IDF 6.1 — PASS; ASan/UBSan dekodera — PASS
  (detect_leaks=0 z powodu środowiska ptrace, bez testu wycieków).
  `git diff --check` — PASS. Bez flash/erase/eFuse.
- Obecny profil RTOS 100Hz daje 10ms próbkowania; szybki transport wejść OPEN.

## Etap 05b — diagnostyka braku zboczy B (2026-10-24)

- Zgłoszenie użytkownika dla 05a: A zmienia się, B=0, sampled edges B=0
  w obu kierunkach. Użytkownik potwierdził półkrok. Nie uznano odbioru wejść.
- Zastąpiono polling zboczy przechwytywaniem ANYEDGE i kolejką 128 próbek.
  Osobne IRQ edges A/B/BTN, komunikat brakującego kanału, raport utraty
  i reset dekodera po overflow. ISR nie loguje ani nie steruje UI.
- Półkrok zachowany; GPIO i pull-up bez zmian. Nowy log odróżnia IRQ edges
  od dawnych sampled edges. Wciąż brak mapowania gestów do UI/playera.
- 84 testy PASS; regresja stałego B nie tworzy kierunku/kroków,
  poprawne półkroki z krótkich impulsów tego samego ms są dekodowane.
- ASan/UBSan dekodera PASS (detect_leaks=0, bez testu wycieków).
  Adapter IRQ/kolejka wymagają sprzętu; brak hostowego testu ich transportu.
- ESP-IDF 6.1 build PASS, obraz 0x3ee00 B. git diff --check PASS.
  Nie wykonano flash/erase/eFuse. ADR 0014.
- HARDWARE_TEST_PENDING: liczniki IRQ obu kanałów, przeciwne delta,
  liczba zdarzeń na ząbek, brak overflow. Przy nadal zerowym B sprawdzić
  fizyczny sygnał GPIO16; przyczyna pozostaje OPEN.

## Wynik diagnostyki enkodera 05b (2026-10-24)

Użytkownik przekazał log IRQ: podczas obrotu A ma m.in. 15/10/35/21 zboczy
na interwał, B stale 0 i zero IRQ; BTN=1. Brak raportu overflow w przesłanym
fragmencie. Odbiór kanału B/kierunku nadal FAILED/OPEN, nie uznano etapu 05.
Ponownie sprawdzono S1E2→IO16 w schemacie i lokalną konfigurację B=16,
HALF_QUADRATURE=y, INTERNAL_PULLUP wyłączone. Półkrok nie zastępuje kanału B.
Następny eksperyment: włączyć istniejący Kconfig wewnętrznych pull-up,
przebudować/wgrać i porównać poziom B oraz IRQ podczas obrotu. Jeśli nadal
stałe zero, pomiar sygnału/ciągłości toru B na sprzęcie. Przyczyna nieustalona.
Zmiana tylko dokumentacyjna; git diff --check PASS, bez nowego buildu/flash.

## Korekta fizycznej mapy enkodera (2026-10-24)

- Użytkownik ustalił rzeczywiste połączenie A/B/BTN=5/6/7 i zmienił B na 6.
  Schemat referencyjny B=16 nie odpowiada używanemu sprzętowi; przyczyna
  braku kanału B wyjaśniona. ADR 0015 zachowuje ten wyjątek jawnie.
- Przesłany log pokazuje IRQ obu kanałów i ROTATE delta=-1/+1, SHORT,
  LONG, PRESSED_ROTATE. Nie ustalono liczby zdarzeń na ząbek/kierunku
  zegarowego ani ciągłości trzymania przycisku po PRESSED_ROTATE.
- Poprawiono domyślne GPIO B=6 w Kconfig/defaults i dokumentację sprzętu.
  Półkrok pozostaje włączony. Nie zmieniono pull-up ani dekodera.
- 84 testy unittest PASS; ESP-IDF 6.1 build PASS; git diff --check PASS.
  Bez flash/erase/eFuse. Mapowanie potwierdzone przez użytkownika,
  pełny etap 05 i adapter gestów do UI nadal nieukończone.

## Odbiór obrotu z przyciskiem (2026-10-24)

Przesłany przez użytkownika log 117335–119815ms pokazuje PRESSED_ROTATE
z delta=+1 i -1 przy BTN=0, bez zwykłych ROTATE. Po zwolnieniu BTN=1
w przesłanym fragmencie nie ma dodatkowego SHORT/LONG. Potwierdzono
poprawne zachowanie tego scenariusza na sprzęcie z pinami 5/6/7.
Wcześniej zaobserwowano również ROTATE obu znaków i SHORT/LONG.
Nie deklarujemy pomiaru maksymalnej szybkości, liczby zdarzeń na ząbek
ani kierunku zegarowego. Następny zakres: adapter gestów A/B do UI.
Zmiana dokumentacyjna; git diff --check PASS, bez nowego buildu/flash.

## Etap 05c — interaktywne sterowanie UI (2026-10-24)

- Czysty adapter gestów A/B: volume delta i toggle przez jawny callback,
  lista przez istniejące API UI, pressed-rotate zmienia tylko widżet HOME.
  Lista: obrót, short confirm, long cancel; pressed-rotate ignorowany.
- Kolejka 32 gestów pomiędzy wejściem i zadaniem ekranu, nieblokujący odbiór;
  przepełnienie jawnie logowane. Bez mutacji UI z ISR/zadania wejścia.
- Aplikacyjny encoder_demo: katalog TEST, volume 50 ograniczone do 0..100,
  PLAY/STOP i syntetyczny widżet, komendy kopiowane przed zastosowaniem.
  Browse/cancel nie zmieniają źródła, confirm zmienia testowy snapshot.
- Nowy domyślny STREAMOO_ENCODER_UI_TEST: HOME od razu, działa do resetu.
  Wyłączenie przywraca stary pokaz 46s. GPIO 5/6/7 i półkrok bez zmian.
- 91 testów unittest PASS (7 nowych); ASan/UBSan adaptera/demonstracji PASS,
  1000 klatek z canary, skrajne delta, A/B, kolejka odmawiająca, pusta lista.
  LeakSanitizer wyłączony pod ptrace (detect_leaks=0), bez testu wycieków.
- ESP-IDF 6.1 reconfigure/build i końcowy build PASS; obraz 0x3e760 B.
  Gałąź starego pokazu dodatkowo skompilowana kompilatorem targetu przy
  wyłączonym makrze interaktywnym (bez osobnego pełnego linku tej konfiguracji).
  Nową klatkę HOME obejrzano na hoście. git diff --check PASS.
- ADR 0016. Bez flash/erase/eFuse. HARDWARE_TEST_PENDING: pełna interakcja,
  jeden ząbek/jeden krok, kierunek, brak overflow obu kolejek, tryby A/B.
- Nie uruchomiono realnego audio/playera, sieci ani NVS. Trwałość po 5s
  i realne komendy playera pozostają do integracji; etap 05 częściowy.

## Korekta kierunku po odbiorze interaktywnego UI (2026-10-24)

Użytkownik potwierdził działanie 05c, zgłosił lewo=volume+, prawo=volume-
i odwrócone przewijanie stacji. Ustawiono ENCODER_REVERSED=y w Kconfig,
sdkconfig.defaults oraz lokalnej konfiguracji buildu. Wspólny dekoder
odwraca zwykły obrót i pressed-rotate; adaptery UI pozostają bez zmian.
Oczekiwane prawo=volume+/następna stacja, lewo=volume-/poprzednia.
ADR 0015 uzupełniony. Piny 5/6/7 i półkrok zachowane.
91 testów unittest PASS (w tym istniejący test odwróconego dekodera).
Odbiór nowego kierunku HARDWARE_TEST_PENDING; bez flash/erase/eFuse.
ESP-IDF 6.1 build PASS; git diff --check PASS.

## Odbiór poprawionego kierunku enkodera (2026-10-24)

Użytkownik potwierdził „Teraz jest OK” po korekcie kierunku w `628ad97`.
Zapisano pozytywny odbiór kierunku w interaktywnym UI: prawo zwiększa
volume/przesuwa do następnej stacji, lewo zmniejsza/przesuwa do poprzedniej.
Nie otrzymano osobnego hasha wgranego obrazu ani logu pomiarowego.
Potwierdzenie nie zamyka otwartych pomiarów szybkości, liczby kroków na ząbek,
trwałości NVS ani integracji realnego playera. Zmiana dokumentacyjna;
`git diff --check` PASS, bez nowego buildu i flashowania.

## Etap 05d — preferencje volume/wizualizacji (2026-10-24)

- Niezależny preferences: format 8B i polityka debounce w czystym C,
  adapter NVS/FreeRTOS z ESP-IDF 6.1 (lokalne API/licencja sprawdzone).
- Odczyt przed startem UI; brak klucza daje 50/spectrum bez tworzenia rekordu
  preferencji. Błąd NVS/formatu wyłącza serwis, UI używa domyślnych;
  brak automatycznego kasowania/reformatowania i brak sekretów.
- UI przekazuje zmiany po każdym geście; identyczne stany są filtrowane.
  Skrzynka latest-wins, czas ostatniej zmiany zachowany także dla A→B→A.
  Osobny worker zapisuje po 5s bez zmian, tylko gdy stan różni się od saved;
  retry po 5s, potwierdzenie dopiero po commit, powrót do saved anuluje zapis.
- Persistowane tylko volume i widżet. Testowe stacje i PLAY/STOP bez zapisu;
  prawdziwe audio/player nadal nie są uruchomione. Startup może wykonywać
  operacje NVS; runtime set_blob/commit wyłącznie w zadaniu preferences.
- 99 testów unittest PASS (8 nowych, w tym 303 pary pól, błędne rekordy,
  debounce, coalescing, revert, retry i długi uptime). ASan/UBSan polityki
  i formatu PASS; detect_leaks=0 pod ptrace, bez testu wycieków.
- ESP-IDF 6.1 reconfigure/build i końcowy build PASS, git diff --check PASS.
  Adapter NVS/FreeRTOS i trwałość po restarcie HARDWARE_TEST_PENDING.
  Nie wykonano flashowania płytki ani erase/eFuse przez agenta. Nowy firmware
  po wgraniu przez użytkownika będzie zapisywał preferencje do NVS.
- ADR 0017. Kontrolowany shutdown/flush nie ma jeszcze właściciela/API;
  restart przed zapisem może utracić zmiany. Test zaniku podczas zapisu,
  zużycia flash i wpływu operacji flash/cache na przyszłe audio pozostaje OPEN.
  Etapy 05/11 nadal częściowe. Następnie podstawowe wyjście I2S (etap 06).

## Odbiór preferencji 05d (2026-10-24)

Użytkownik potwierdził „Działa” po instrukcji testu zapisu volume/widżetu,
odczekania komunikatu Saved i restartu. Zapisano pozytywny odbiór tej
funkcji według kontekstu wersji `215d49a`; bez osobnego hasha wgranego
obrazu i logu pomiarowego. Nie rozszerzamy odbioru na test zaniku podczas
zapisu, zużycie flash, błędy NVS ani kontrolowany shutdown/flush.
Kolejny zakres: podstawowe wyjście I2S/PCM5102A etapu 06.
Zmiana dokumentacyjna; git diff --check PASS, bez nowego buildu/flash.

## Etap 06a — I2S/DMA i test PCM5102A (2026-10-24)

- audio_output: natywny esp_driver_i2s (API/licencja Apache-2.0 lokalnego
  ESP-IDF 6.1 sprawdzone). Master I2S0, Philips stereo 48kHz/32-bit,
  slot32, nominalne BCLK 3.072MHz/WS48kHz, bez MCLK, GPIO48/47/21.
- 4 bufory DMA po 240 ramek, jawny preload zer przed enable; callback
  uzupełnia zakończony bufor. Bez alokacji, flash, blokującego write, logów,
  float i FFT w wydawaniu próbek. I2S/GDMA ISR IRAM-safe włączone.
- Oryginalny skończony generator: 10s ciszy/L1k/R2k/oba1k z przerwami,
  peak1036 w ekwiwalencie int16 (~-30dBFS), rampy 50ms. Niezależny od
  TEST VOL/PLAY. Po końcu zera, odpływ DMA i disable/delete poza ISR.
- Nowy STREAMOO_I2S_SMOKE_TEST domyślnie y. UI/enkoder i NVS działają
  równolegle; nie połączono jeszcze prawdziwego playera/DSP/strumieni.
- 107 testów unittest PASS (8 nowych, pełne 480000 ramek, częstotliwości,
  izolacja, RMS/DC/peak, granice ramp, chunk invariance, canary i argumenty).
  ASan/UBSan generatora PASS, 2100 bloków; detect_leaks=0 pod ptrace,
  bez testu wycieków. Driver DMA i analogowy DAC wymagają sprzętu.
- ESP-IDF 6.1 reconfigure/build i końcowy build PASS, git diff --check PASS.
  ELF: tone_dma_fill 0x40377508, tone_fill 0x40377518, audio_dma_sent
  0x40377618 (IRAM), tone/wave 0x3fc96fd8/0x3fc96fdc (DRAM). Generator
  nie ma wywołań helperów według objdump. Nie wykonano flash/erase/eFuse.
- ADR 0018. HARDWARE_TEST_PENDING: rzeczywiste połączenia/strapping modułu,
  kanały i dźwięk, clock/jitter, brak słyszalnych zakłóceń podczas UI/NVS,
  max_fill i liczba buforów. Statystyki callbacku nie są testem underrun.
- Pipeline/kodeki/player, rzeczywisty volume ramp, FIFO i DSP nadal OPEN.

## Odbiór testu audio 06a (2026-10-24)

Użytkownik potwierdził „działa” po instrukcji odsłuchu skończonej sekwencji
L1k/R2k/oba1k w kontekście wersji `edb7c63`. Zapisano pozytywny odbiór
podstawowego wyjścia I2S/PCM5102A. Nie otrzymano osobnego hasha obrazu,
końcowego logu audio_probe ani wyników pomiarów zegara/czasów callbacku.
Potwierdzenie nie jest pomiarem jitter, underrun, obciążenia, zachowania
przy zapisach NVS ani dowodem działania pełnego pipeline/playera.
Następny zakres audio: rzeczywista regulacja gain z rampą oraz sterowanie
start/stop przez oddzielnego właściciela stanu. Zmiana dokumentacyjna;
git diff --check PASS, bez nowego buildu/flash.

## Koniec pracy i punkt wznowienia (2026-10-24)

Na prośbę użytkownika zakończono pracę na dziś. Aktualny stan i kolejny
zakres 06b (rzeczywisty gain/rampa i PLAY/STOP) zapisano w RESUME.md.
Ostatni odebrany firmware: 06a `edb7c63`, odbiór `80e146c`.
Zmiana dokumentacyjna; git diff --check PASS, bez nowego buildu/flash.

## Etap 06b — gain z rampą i sterowanie tonem (2026-10-25)

- Wznowiono z RESUME: odebranego testu I2S nie powtarzano od zera.
- Nowy własny audio_gain: tabela Q16 0..100, 0 mute/100 unity, proponowana
  krzywa -59.4..0dB dla 1..100, liniowa rampa amplitudy 2400 ramek/50ms.
  Retarget od aktualnej wartości; powtarzany cel nie resetuje rampy.
  PCM stereo32, wspólny gain L/R, bez przesterowania/UB dla ekstremów int32.
- Osobny aplikacyjny właściciel audio_control: czysty model i kolejka16
  VolumeDelta/Toggle. UI dostaje snapshot, nie zmienia rzeczywistego stanu
  optymistycznie. Pełna kolejka odmawia. ISR posiada wyłącznie fazę/rampę,
  z zadania pobiera chroniony target; bez heap/flash/IO/FFT/float/logów.
- Nowy STREAMOO_AUDIO_CONTROL_TEST domyślnie y: start STOP z volume z NVS,
  ciągły ton1kHz stereo o peak~-30dBFS przed gain. PLAY/STOP i VOL są teraz
  rzeczywiste dla tego źródła. STOP rampuje do ciszy przy włączonym I2S.
  D12 autoplay radia nadal OPEN. Wyłączenie przywraca stary test06a.
- Katalog i widżety nadal TEST, nie analiza PCM ani prawdziwe stacje.
  Browse/cancel/widżety nie zmieniają gain; confirm zmienia testową nazwę.
- NVS nadal ma jednego producenta UI. volume_revision zachowuje informację
  o sklejonych zmianach A→B→A; submit_changed resetuje debounce także dla
  tej samej wartości końcowej, start terminu przy obserwacji przez UI.
- 121 testów unittest PASS (14 nowych): model/komendy i clamp/revision,
  krzywa, retarget, 2400 ramek, ekstremalne PCM, stereo/chunk/canary, źródło
  ciągłe i zewnętrzny właściciel UI bez optymistycznych zmian. ASan/UBSan
  gain/model/source oraz 8 testów UI/control PASS; detect_leaks=0 pod ptrace,
  bez testu wycieków. FreeRTOS/DMA i odsłuch wymagają sprzętu.
- ESP-IDF6.1 reconfigure/build i końcowy build PASS, obraz0x4a5f0 B.
  Gałąź bez AUDIO_CONTROL_TEST: main/display dodatkowo skompilowane
  kompilatorem targetu (bez osobnego pełnego linku tej konfiguracji).
  ELF: controlled_fill0x403774fc, sine_fill0x40377534, gain_target0x40377598,
  gain_process0x403775f4 w IRAM; render/wave/volume_gain w DRAM. Objdump:
  brak wywołań helperów w gain. git diff --check PASS. Bez flash/erase/eFuse.
- ADR0019; RESUME zaktualizowany. HARDWARE_TEST_PENDING: rzeczywisty poziom,
  płynność zmian i PLAY/STOP, max_fill, działanie UI/NVS podczas audio.
  Brak pomiaru jitter/latencji/underrun; etap06 i cały projekt nieukończone.
- Kolejno: odbiór06b, następnie PCM FIFO/tap przed gain i przygotowanie
  źródła PCM/dekoderów; pełny player, sieć i DSP nadal OPEN.

## Odbiór sterowanego audio 06b (2026-10-25)

Użytkownik potwierdził „Działa, ładnie” po instrukcji testu rzeczywistego
VOL/gain z rampą i PLAY/STOP na źródle testowym, w kontekście `a62eeb6`.
Zapisano pozytywny odbiór funkcjonalny. Bez osobnego hasha wgranego obrazu,
logu max_fill i pomiarów latencji/jitter/underrun; nie rozszerzamy odbioru
na testy obciążenia ani niezależny pomiar braku trzasków.
Kolejny zakres: PCM FIFO i tap przed gain, przygotowanie źródła PCM/dekoderów.
Zmiana dokumentacyjna; git diff --check PASS, bez nowego buildu/flash.

## Etap 06c — PCM FIFO i tap przed gain (2026-10-25)

- Nowy własny pcm: stałe bloki stereo32/48kHz, SPSC release/acquire,
  numery sekwencji i poprawne wrap liczników. Brak nowej biblioteki/kodu
  zewnętrznego. GCC/ESP-IDF6.1: wyrównane 32-bit load/store audytowane
  w assemblerze; ogólny ATOMIC_INT_LOCK_FREE był zbyt szerokim warunkiem
  przy fladze -mdisable-hardware-atomics i został zastąpiony właściwym
  sprawdzeniem rozmiaru/wyrównania plus audytem kodu wynikowego.
- Producent tonu poza ISR. Raw FIFO32 bloków/160ms (61440B PCM+128B tagów),
  tap4 bloki/20ms (7680B PCM+16B tagów), wszystko w wewnętrznym RAM.
  Tap jest po raw FIFO, przed gain. Po gain pozostaje DMA20ms.
- Rezerwa10 bloków rozpoczyna 50ms fade na rzeczywistym PCM, potem zera
  i BUFFERING; powrót po >=24 blokach/120ms. Awaryjny twardy brak ma
  osobny licznik i podtrzymanie ostatniej próbki tylko na czas fade.
- Tap drop-newest nie czeka na analizę. Sequence ujawnia luki; osobny
  odbiorca liczy debug peak, bez FFT/RMS. Brak podłączenia realnych widżetów.
  Przy aktywnym źródle tap_peak16=1036 niezależnie od VOL i STOP.
- UI pokazuje BUF podczas ochronnego wyciszenia/podbuforowania przy PLAY.
  Opcjonalny PCM_STALL_TEST (pauza400ms po3s pierwszego PLAY) oraz
  PCM_TAP_DELAY_MS (wymuszenie strat analizy) domyślnie n/0.
- 132 testy unittest PASS (11 nowych), w tym 20000 bloków w dwóch wątkach,
  full/empty/wrap, tap0/50/100, fade/rebuffer, STOP, dropout tap bez zmiany
  audio i awaryjny brak. ASan/UBSan 11 testów pcm PASS; detect_leaks=0
  pod ptrace, bez testu wycieków. ThreadSanitizer nie uruchomiony.
- ESP-IDF6.1 końcowy build PASS, obraz0x4ac10 B; git diff --check PASS.
  Gałęzie stall/tap-delay oraz wyłączonego control skompilowano dodatkowo
  kompilatorem targetu, bez osobnego pełnego linku tych konfiguracji.
  ELF: pcm_fill0x403775a8, count0x403793e0, push0x403793fc, pop0x40379484
  w IRAM. storage tap/PCM0x3fc9a530/0x3fc9c340 w wewnętrznym BSS, meta
  w DRAM. Brak helperów __atomic w obiekcie. Memset/memcpy dostępne w ROM.
- ADR0020, RESUME zaktualizowany. Bez flash/erase/eFuse. HARDWARE_TEST_PENDING:
  zwykły odsłuch/UI/NVS, celowe zagłodzenie i przeciążenie tap, max_fill,
  brak nieoczekiwanych low/hard. Nie deklarujemy detekcji każdego DMA replay,
  jitter ani zmierzonej latencji. Pipeline stałego formatu, bez generacji/flush
  źródeł, dekoderów, sieci i pełnego playera.
- Dalej: odbiór06c, następnie wybór i PoC dekodera z przeglądem API/licencji
  ESP-IDF6.1 oraz adapter formatu PCM. DSP właściwy pozostaje etapem07.

## Wynik pracy PCM/tap 06c na sprzęcie (2026-10-25)

Użytkownik przesłał log w kontekście wersji `368c724`, zakres około35–51s
od boot: buffers7005→10005 (po1000/5s), max_fill129us, invalid0;
queued28, mode1 ACTIVE, low0, hard0, starts1; tap_drop0, tap_gaps0,
tap_blocks7003→10003, tap_peak16 stałe1036. Zapis NVS volume38/widget1
przy39.472s nie zmienił powyższych liczników w przesłanym fragmencie.
Heap internal free258663B/min222428B/largest180224B stabilny w dwóch
odczytach; PSRAM free8377564B, health stack high-water1920B.
Potwierdzono stabilną pracę PCM/tap w tym przedziale oraz poprawny poziom
przed gain przy zaobserwowanym ustawieniu. Brak osobnych porównań VOL0/50/100,
odsłuchu i testów wymuszonego niedoboru/tap-delay; pozostają OPEN.
129us to maksimum mierzonego callbacku renderowania w tym przebiegu, nie
WCET całego systemu ani pomiar latencji IRQ, jitter i DMA replay.
Zmiana dokumentacyjna; git diff --check PASS, bez nowego buildu/flash.

## Profil odbioru PCM stress (2026-10-25)

- Po stabilnym logu06c przygotowano bieżący lokalny sdkconfig/build z
  PCM_STALL_TEST=y oraz PCM_TAP_DELAY_MS=100. Normalne domyślne n/0
  pozostają niezmienione; odtwarzalny fragment config/pcm_stress.defaults.
- Algorytm PCM/gain bez zmian. Dodano startowy komunikat jawnie pokazujący
  flagi testu, instrukcję PCM_STRESS_TEST.md i ostrzeżenie w RESUME o aktywnym
  lokalnym profilu. Rozszerzono istniejący ADR0020 o sposób odbioru.
- Oczekiwane po pierwszym PLAY: jedna celowa przerwa, samoczynny powrót,
  low1/hard0/starts2, potem stabilny ton przy rosnących tap_drop/tap_gaps.
  Wymaga odsłuchu i logu użytkownika; HARDWARE_TEST_PENDING.
- 132 testy unittest PASS, ESP-IDF6.1 build z obiema flagami PASS,
  git diff --check PASS. Bez flash/erase/eFuse. Po teście przywrócić lokalnie
  n/0 przed zwykłym buildem i rozpoczęciem dekoderów; NVS pozostawić.

## Wynik profilu PCM stress na sprzęcie (2026-10-25)

Użytkownik przekazał log w kontekście `3659b69`:

- Przed wymuszeniem (15.882s): mode1/queued28, low0/hard0/starts1.
- TEST producer stall16.202s → resumed16.602s: dokładnie400ms według logu.
- Od20.882s do70.882s: mode1/queued28, low1/hard0/starts2 bez narastania.
  buffers4005→14005, co1000/5s. max_fill113us, invalid0.
- tap_drop3201→11201, tap_blocks800→2800, tap_gaps3183→11183:
  celowo przeciążony odbiorca traci bloki, PCM nie zgłasza kolejnych low/hard.
  tap_peak16 po powrocie stałe1036, także po LONG51.172s. Różnica drop/gaps
  nie jest błędem: luki wykrywamy przy odebraniu kolejnej sekwencji.
- Saved volume85/widget1 przy14.112s. Heap internal free258663B,
  min222428B/largest180224B stabilne; PSRAM free8377564B; health stack1920B.

Potwierdzono licznikami rebuffer/powrót i izolację przeciążonego tap w tym
przebiegu. Brak słownego potwierdzenia odsłuchu; nie uznajemy jeszcze braku
trzasków/dodatkowych przerw ani pełnych pomiarów jitter/latencji/DMA replay.
Lokalny sdkconfig przywrócono do PCM_STALL_TEST=n / TAP_DELAY_MS=0;
fragment stresu pozostaje opcjonalny, NVS nie kasowano. Firmware wgrany
na płytkę nie został przez agenta zmieniony. Zmiana dokumentacyjna;
git diff --check PASS. Normalny profil przebudowany bez nowych zmian kodu.

## Odbiór odsłuchu PCM stress (2026-10-25)

Na pytanie, czy była tylko jedna łagodna przerwa, a potem ton grał czysto,
użytkownik potwierdził: „Tak, grał czysto”. Łącznie z poprzednim logiem
zamknięto odbiór tego scenariusza: kontrolowany niedobór/powrót oraz
przeciążony tap bez kolejnych słyszalnych zakłóceń. Nie jest to pomiar
jitter/latencji, test wielogodzinny ani gwarancja dla dowolnego obciążenia.
Normalny lokalny profil n/0 pozostaje przywrócony. Dalej: wybór/PoC dekodera
i adapter PCM po audycie API/licencji/ESP-IDF6.1. Zmiana dokumentacyjna;
git diff --check PASS, bez nowego buildu/flash.

## Etap06d — pierwszy MP3 i konwersja PCM (2026-10-25)

- Zakończono badanie wersji/API/licencji przed integracją: oficjalny
  espressif/esp_audio_codec2.6.2, wymóg IDF>=4.4, biblioteki S3, dokładny
  pin i dependencies.lock (hash a5b9275af74247e3ce536d16a7080e4d77207a185b63e1bea368eb4d57176c2b).
  Licencja Espressif Modified MIT (ESP-only), MP3 wg vendor Apache2/Android.
  Zachowano notices/teksty; bez kodu GPL w tej integracji.
- Własny decoder rejestruje tylko MP3 i używa Simple Decoder; fragmenty
  nie muszą pokrywać ramek. Jawne consumed/produced/needed/vendor status,
  metadane i close, jeden właściciel zadania. ELF zawiera MP3 open/decode,
  bez AAC/Opus/FLAC open/decode. Build/link to nie test działania dekodera.
- Własny pcm_adapter:16LE mono/stereo48kHz do stereo32, wszystkie wartości
  s16 bez signed-shift UB, byte loads dla unaligned, odrzucanie unsupported
  rate/bits/channels bez resamplingu. Pętla PCM zawija także w środku bloku.
- Oryginalny klip matematyczny4s, MP3 CBR128k/48k stereo,64512B/168 ramek.
  FFmpeg/libmp3lame wyłącznie narzędziami hosta; brak cudzych nagrań/kodu
  w firmware. Manifest z hashami/narzędziem, źródło generatora zachowane.
- Domyślny MP3_FIXTURE_TEST=y: przygotowanie PCM w1MiB PSRAM przed I2S,
  chunk257B/scratch8192B, stos24576B, potem close dekodera. Producent
  konwertuje do SRAM FIFO; ISR bez dekodera/PSRAM/flash/allocacji. To batch
  predecode, nie test realtime streamingu. Log ramek, formatu, peak, czasów
  i stack_hwm wymaga sprzętu. Błąd nie maskuje się zastępczym tonem.
- UI start STOP/volume NVS, tytuł TEST plik MP3, sekwencja L/R/stereo z
  zakodowaną ciszą zapętlana zamiast sinusa. Tap peak zmienny, widżety
  i stacje nadal TEST. Flagi stress pozostają n/0.
- 141 testów hostowych PASS (9 nowych), w tym cały zakres65536 s16,
  mono/stereo/bounds/unaligned, pętla i hash/nagłówki fixture. ASan/UBSan
  adaptera i pętli PASS; detect_leaks=0 pod ptrace, bez testu wycieków.
  Vendorowe binaria nie są uruchamiane na hoście.
- ESP-IDF6.1 reconfigure/build i końcowy build PASS, git diff --check PASS.
  Sieć ograniczonego środowiska wymagała pobrania pakietu przez Component
  Manager z dostępem do oficjalnego rejestru; zależność zainstalowana
  niezmieniona, LICENSE porównany z audytowaną paczką. Bez flash/erase/eFuse.
- ADR0021. HARDWARE_TEST_PENDING: MP3 PoC ESP_OK, frame count/metadane,
  czas/stack/heap, odsłuch kilku cykli i działanie VOL/PLAY/STOP/UI/NVS.
  Nie deklarujemy jeszcze AAC/HE-AAC ani innych kodeków,44100/resamplingu,
  streamingu/CPU realtime, generacji/seek/cancel i pełnego playera.
- Dalej po odbiorze: AAC-LC PoC, następnie dekodowanie porcjami bez
  przechowywania całego PCM pliku oraz adapter resamplingu według pomiarów.

## Wynik MP3 PoC06d na ESP32-S3 (2026-10-25)

Użytkownik przekazał log w kontekście `3f761ae`:

- MP3 PoC ESP_OK przy1.378s: input64512B, pcm_frames193536 — dokładnie
  referencja hosta/168 ramek MPEG. rate48000, channels2, bits16, peak16=985.
  Ta próba potwierdza także fragmentację257B i koniec tego pliku.
- decode_us337274, wall_us501083, stack_hwm22744. To metryki batch PoC,
  nie pomiar CPU streamingu ani wymiarowanie stosu pozostałych kodeków.
- Profil producer_stall=OFF/tap_delay0, source embedded MP3, początkowy STOP.
  buffers1006→11008 do56.428s; max_fill104→133us, invalid0; queued28,
  mode1, low0/hard0/starts1, tap_drop0/tap_gaps0. Peak tap0/984/985
  jest zgodny z ciszą i tonami w stratnym pliku.
- Heap internal free237203B/min176440B/largest131072B stabilny w próbkach;
  PSRAM free/min7328984B, largest7208960B. Saved volume83/widget1 przy50.638s
  bez wzrostu liczników niedoboru w późniejszym odczycie.
- Około15.9–16.9s jest fragment nieczytelnego tekstu konsoli, przyczyna
  nieustalona (OPEN). Dalsze logi i liczniki działają; obserwować powtarzalność.

Potwierdzono wykonanie dekodera na6.1/S3 i stabilną pracę przygotowanego PCM
w tym przebiegu. Użytkownik nie opisał jeszcze odsłuchu kanałów/cyklu.
Nie deklarujemy bitowej identyczności PCM, pomiaru jitter ani gotowego
streamingu/obsługi kolejnych formatów. Bez osobnego hasha wgranego obrazu.
Zmiana dokumentacyjna; git diff --check PASS, bez nowego buildu/flash.

## Etap06e — AAC-LC ADTS (2026-10-25)

- Użytkownik wyjaśnił nieczytelny fragment wcześniejszego UART: poruszył przewodem.
  Polecił kontynuować; nie zmieniano UART ani nie powtarzano testu MP3.
- Wspólny adapter decoder dla MP3 i AAC-LC, esp_audio_codec nadal dokładnie2.6.2.
  AAC z ADTS, AAC-Plus wyłączony. Wybór jednego źródła w Kconfig: AAC-LC
  (domyślny), MP3 lub sinus. Lokalny obraz końcowy: AAC-LC, stress n/0.
- Własny klip AAC37039B,189 ramek ADTS, referencja193536 ramek PCM.
  Nadal batch decode przed I2S do1MiB PSRAM; bez streaming decode/resamplingu.
  ISR/FIFO/gain bez zmian. HE-AAC i pozostałe formaty OPEN.
- 142 testy hostowe PASS. ESP-IDF6.1 build/link wszystkich trzech źródeł PASS:
  MP3 0x87400B, sinus 0x4acd0B, końcowy AAC 0x808b0B; partycja1MiB.
- Licencje i attribution AAC sprawdzone w przypiętym pakiecie i oficjalnym
  COPYRIGHT Espressif; zachowane Espressif Modified MIT ESP-only i Apache2.0.
- ADR0022. Odbiór AAC na ESP32-S3/odsłuch OPEN. Nie wykonano flash/erase.

## Odbiór logu06e — AAC-LC na ESP32-S3 (2026-10-25)

Firmware e73523d, log użytkownika: AAC-LC PoC ESP_OK, input37039B,
pcm_frames193536 (dokładnie referencja),48000Hz/stereo16, peak1043,
decode_us309922, wall_us406630, stack_hwm22552B. Dekodowanie pliku PASS.
Profil normalny: producer_stall OFF, tap_delay0, start STOP, volume83 z NVS;
LONG w8.255s. Do36.335s PCM queued28/ACTIVE, low0/hard0/starts1,
invalid0, tap_drop0/tap_gaps0, max_fill129us. Bufory rosną1000/5s.
Heap internal235415B i PSRAM7328984B stabilne w próbkach10–30s;
minimum internal188236B, PSRAM7293140B. Zmienny peak0..1043 odpowiada
klipowi z ciszą. Brak błędów w dostarczonym logu; nie jest to pomiar jitter
ani test długotrwały. Odsłuch L/R/oba i brak słyszalnych zakłóceń OPEN.
Zmiana wyłącznie dokumentacji; nie powtarzano buildu/testów kodu.

## Odbiór odsłuchu06e — AAC-LC (2026-10-25)

Po pytaniu o czysty odsłuch kanałów lewy/prawy/oba użytkownik potwierdził:
„tak, wszystko grało ładnie”. Odsłuch AAC-LC zaliczony. W połączeniu
z wcześniejszym logiem zamyka odbiór PoC06e; nie jest to odbiór streaming
ani HE-AAC. Następny zakres: dekodowanie w trakcie odtwarzania z backpressure,
bez przechowywania całego rozkodowanego klipu w PSRAM.
Zmiana dokumentacyjna; kod i obraz firmware bez zmian, bez powtarzania testów.

## Etap06f — incremental file decode (2026-10-25)

- Zamiast1MiB rozkodowanego PCM:257B wejścia i8192B wyrównanego PCM,
  stan/cursory w internal RAM. Jedyny właściciel w zadaniu audio_control.
  Producent pobiera240 ramek tylko przy miejscu w FIFO; zachowuje ogony
  wejścia/pakietu i resetuje parser/dekoder w EOF, bez ponownego open.
- API reset sprawdzone w nagłówku esp_audio_codec2.6.2; zależności i licencje
  bez zmian. Nadal plik we flash, bez sieci/resamplingu/HE-AAC. ISR/FIFO/
  gain/tap bez zmian; STOP nadal konsumuje źródło z wyciszeniem.
- Kontrole błędów/progresu/formatu/liczby ramek, błąd zatrzaskiwany; niepełny
  blok nie trafia do FIFO. Diagnostyka cycles/frames/calls/max_read_us/stos.
- 152 testy hostowe PASS (10 nowych przypadków kontrolowanego backendu),
  ASan/UBSan pump/konwertera PASS, LeakSanitizer wyłączony (brak testu wycieków).
  ESP-IDF6.1 build/link AAC-LC, MP3 i sinus PASS; końcowy lokalny profil AAC,
  stress n/0. Walidator GPIO/platform i diff --check PASS.
- ADR0023. Odbiór06f na S3 OPEN: stabilny FIFO/heap/stos, czas read i reset
  podczas DMA oraz odsłuch kilku cykli L/R/oba, VOL/PLAY/STOP. Wynik06e batch
  nie zalicza tych nowych warunków. Nie wykonywano flash/erase.

## Odbiór06f — AAC incremental decode (2026-10-25)

Użytkownik przekazał log dla3b18324 i potwierdził „Dżwięk OK”.
AAC-LC streaming source ESP_OK, input37039B, work_bytes8528 bez pełnego
klipu PCM. cycles1..5, każdy frames193536, ok1; max_read_us4034,
stack_hwm22728B. LONG w6.005s, volume83 z NVS, stress OFF/0.
Do20.955s queued27..30/ACTIVE, low0/hard0/starts1, invalid0,
tap_drop0/tap_gaps0, max_fill130us. Zmienny peak z ciszą zgodny z klipem.
Heap internal211223B (min175568, largest131072) i PSRAM8341720B
(min8341720, largest8257536) stabilne w próbkach10.845/20.845s.
Względem batch06e wolna PSRAM wzrosła o1012736B; internal zmalała
z235415 do211223B (dekoder pozostaje otwarty i ma stan roboczy).
max_read4034us dotyczy pull240 w zadaniu, max_fill130us callbacku render;
nie są pomiarem całego ISR/jitter ani dowodem deadline przy innym obciążeniu.
Odbiór AAC decode/reset podczas DMA i odsłuch zaliczone. To krótki test,
nie test długotrwały; MP3 incremental, nowe próby VOL/STOP, sieć i HE-AAC
nie wynikają z tego logu. Następny zakres: oddzielny interfejs źródła
skompresowanego i bufor z backpressure. Nie powtarzano buildu/testów kodu.

## Koniec dnia — checkpoint 2026-10-25

Na prośbę użytkownika zakończono pracę po odbiorze AAC incremental06f.
Kod3b18324, odbiór cfd4917. Szczegóły i kolejność wznowienia w RESUME.md.
Lokalny profil AAC-LC, stress OFF/tap_delay0. Kolejny etap nie rozpoczęty:
interfejs źródła skompresowanego i bufor z backpressure, potem reader sieciowy.
Checkpoint wyłącznie dokumentacyjny; poprzednie wyniki testów bez zmian.

## Etap06g — interfejs źródła i bufor skompresowany (2026-10-31)

- Wznowiono z checkpointu8df80f5; AAC06f nadal odebrane. Nowy własny
  komponent compressed_source bez zewnętrznych zależności i bez RTOS:
  DATA/WAIT/END/ERROR, partial write bez nadpisywania, finish/clear.
- Test: feeder porcji do997B →4096B PSRAM ring → callback źródła → decoder
  → PCM FIFO. Mały bufor wymusza zawijanie i backpressure, nie ustala
  produkcyjnego bufora256KiB ani progów sieci. Jeden właściciel audio_control;
  nowy reader w innym zadaniu wymaga synchronizacji/generacji, nadal OPEN.
- Pump nie zna już adresu pliku. WAIT zachowuje częściowy blok PCM i nie
  zmienia output; do FIFO tylko kompletny blok. Wstrzymanie producenta PCM
  przy WAIT pozostawia działającą obsługę komend i istniejący rebuffer.
  Ostatni bajt źródła zatrzymany do finish, EOS przekazywane z danymi.
- ISR/gain/tap/piny/NVS bez zmian. Zależności/licencje bez zmian, brak nowego
  kodu stron trzecich. Brak sieci, HE-AAC i resamplingu.
- 161 testów hostowych PASS. ASan/UBSan:15 przypadków pump/konwertera/źródła
  i4 pierścienia PASS; LeakSanitizer wyłączony, brak testu wycieków.
- ADR0024, procedura HARDWARE_BRINGUP. Odbiór06g na S3 OPEN: cycles/frames,
  liczniki compressed/backpressure/waits, PCM/heap/stos i odsłuch L/R/oba
  oraz VOL/PLAY/STOP. Nie wykonywano flash/erase.

Weryfikacja build06g: ESP-IDF6.1/esp32s3 build/link AAC-LC0x80d00B,
MP3 0x87840B, sinus0x4acd0B PASS. Końcowy obraz AAC-LC, stress OFF/0,
partycja aplikacji1MiB (AAC około50% wolnej). Walidator GPIO/platform
oraz git diff --check PASS. Brak nowej alokacji/dekodera/I/O w callbacku I2S.

## Odbiór logu06g — AAC przez bufor skompresowany (2026-10-31)

Log użytkownika dla c451643: cycles1..10/ok1,193536 ramek w każdym cyklu,
compressed3839B w próbkach po przewinięciu, backpressure160→1662, waits0.
Kontrola przepływu działa bez utraty ramek; waits0 jest oczekiwane dla tego
feederu i nie stanowi sprzętowego testu przerwy w dostarczaniu danych.
max_read3829us, stack_hwm min22728B; max_fill133us, invalid0.
PCM queued28/ACTIVE, low0/hard0/starts1, tap_drop0/tap_gaps0 do41.006s.
Heap internal209143B (min173488, largest131072), PSRAM8337620B
(min8337620, largest8257536) stabilne w próbkach10–40s.
LONG13.766/24.346s, obroty enkodera oraz zapis NVS volume78/widget1
w26.176s nie przerwały postępu buforów. Sam log nie potwierdza słyszalnej
zmiany głośności/STOP ani czystego odsłuchu. Odsłuch06g OPEN.
Brak błędów w przekazanym fragmencie; to krótki test, nie test jitter/deadline
ani wielozadaniowego readera/sieci. Zapis tylko dokumentacji, bez ponownego
buildu/testów kodu i bez flashowania.

## Odbiór odsłuchu i sterowania06g (2026-10-31)

Na pytanie o czysty dźwięk i prawidłowe VOL/PLAY/STOP użytkownik odpowiedział
„tak”. Odsłuch i sterowanie06g zaliczone, wraz z wcześniejszym logiem zamyka
to odbiór testowego źródła/bufora AAC. WAIT na S3, niezależny reader,
sieć, MP3 przez nowy bufor oraz test długotrwały nadal osobno OPEN.
Następny krok: adapter komunikacji/synchronizacji dla niezależnego readera
przy zachowaniu jednego właściciela dekodera. Zmiana tylko dokumentacyjna.

## Etap06h — niezależny reader i generacje (2026-10-31)

- Nowy task-only buffered_source serializuje byte_ring mutexem FreeRTOS,
  nie portMUX. Read nie blokuje (contention→WAIT), write/finish z tokenem,
  rewind zmienia generację i czyści pod mutexem; stare dane/EOF odrzucone.
  Snapshot/rewind/stop mają timeout5ms w tickach, minimum1tick.
- fixture_reader priorytet3/stos4096B posiada offset i chunk997B. Audio_control
  nie zasila źródła, zachowuje jedyną własność dekodera/pump/PCM. Mutex nie
  obejmuje przygotowania chunk ani I/O źródła; nadal oryginalny plik we flash.
- Release najpierw stop/notify i ack readera do1s, dopiero potem free/destroy.
  Przy timeout zachowuje żywe bufory. Nie wymusza usuwania taska używającego
  pamięci. Ścieżki timeout/join i długotrwała praca na S3 nadal OPEN.
- API i licencję mutex/semafor/notifications sprawdzono w ESP-IDF6.1:
  FreeRTOS-Kernel MIT, już część SDK. Zależności/lock/notices bez zmian;
  brak kopiowania kodu stron trzecich. ISR/gain/tap/piny/NVS bez zmian.
- 168 testów hostowych PASS;7 nowych przypadków rzeczywistego adaptera
  z pthread shim, m.in. równoległy transfer200003B przez31B ring oraz
  rewind pomiędzy pobraniem tokena a publikacją. ASan/UBSan tych7 PASS;
  LeakSanitizer/ThreadSanitizer nie uruchamiano. Nie jest to hostowy test
  FreeRTOS scheduler ani firmware reader_task/lifecycle ack.
- ADR0025. Odbiór06h na S3 OPEN: cycles/generation/stats_ok, PCM/heap/stosy,
  czysty odsłuch i VOL/PLAY/STOP. WAIT przy starcie/EOF dopuszczalny,
  low0/hard0/invalid0 oczekiwane. Brak sieci/reconnect/HE-AAC/resamplingu.
  Nie wykonywano flash/erase.

Build06h ESP-IDF6.1/esp32s3: AAC0x81230B, MP3 0x87d70B, sinus0x4acd0B
PASS. Końcowy obraz AAC-LC, stress OFF/tap_delay0, partycja1MiB około50%
wolnej. Walidator GPIO/platform oraz git diff --check PASS. Callback render
oraz pcm_fill pozostają w IRAM, adapter źródła poza ścieżką IRQ.
