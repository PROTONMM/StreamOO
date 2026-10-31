# Status implementacji

Bieżący punkt wznowienia: [RESUME.md](RESUME.md).

| Etap | Stan | Dowód |
|---|---|---|
| 01 zakres | wymagania zebrane | dokumentacja |
| 02 schemat i profil | walidator i menuconfig zaimplementowane; HARDWARE_TEST_PENDING | ADR 0001; 5 testów hostowych, 2332 konfiguracje; build PASS |
| 03 szkielet | baza zaimplementowana; odbiór sprzętowy i integracyjny OPEN | ESP-IDF v6.1 build PASS; ADR 0002 |
| 04 UI/SSD1322 | HOME i lista odebrane na OLED; końcowe strojenie fontu OPEN | ADR 0012; odbiór użytkownika |
| 05 encoder | dekoder/IRQ i gesty odebrane; interaktywny adapter odebrany w trybie A; preferencje NVS odebrane; shutdown/flush OPEN | ADR 0013–0017; 99 testów PASS |
| 06 audio | I2S/gain/PCM stress odebrane; MP3 decode/PCM potwierdzone logiem; AAC-LC PoC odebrane; AAC incremental file decode06f odebrane logiem i odsłuchem; źródło/bufor AAC06g odebrane logiem, odsłuchem i sterowaniem; niezależny reader AAC06h odebrany; player06i: kilka stacji i 44,1/48kHz odebrane; rozszerzone testy OPEN | ADR0018–0025/0034–0035; 248 testów hostowych PASS; build PASS |
| 07 DSP | RMS/peak działa, strojenie odłożone; widmo live07e gotowe do odbioru, ESP-DSP/wydajność OPEN | ADR0037–0042;303 testy i build PASS |
| 08 stacje | lista z katalogu i wybór odebrane; import live/reboot i edycja ID OPEN | ADR0033 |
| 09 sieć | AP/STA/IP/AP-off potwierdzone logiem; mDNS OPEN | ADR0031; build PASS |
| 10 WWW | formularze Wi-Fi/upload bez dodatkowego logowania; HOME/IP odebrane; lista z katalogu OPEN | ADR0032; 222 testy PASS; Chromium PASS |
| 11 storage/OTA | NVS preferencji odebrane; profile NVS/LittleFS katalog gotowe do odbioru; OTA OPEN | ADR0017/0031 |
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

## Odbiór06h — niezależny reader AAC (2026-10-31)

Użytkownik przekazał log6031941 i potwierdził: „Dźwięk czysty, działa jak
poprzednio”. Reader generacje1..7, każda input37039B, min stack_hwm1980B.
Pump cycles1..7/ok1, frames193536/cykl, generation2..8, stats_ok1, stale0.
WAIT1→7 i compressed0 po rewind są normalne; brak utraty ciągłości audio.
Backpressure317→2289. max_read3854us, min stack_hwm audio_control22664B.
PCM queued28..29/ACTIVE, low0/hard0/starts1, invalid0, tap_drop/gaps0,
max_fill131us w próbkach do25.946s. Log cykli trwa do28.986s.
Heap internal204743B (min169088, largest126976) i PSRAM8337620B
(min8337620, largest8257536) stabilne w próbkach10.846/20.846s.
Obroty/LONG i zapisy NVS volume80 oraz87 nie przerwały pracy; użytkownik
potwierdził zachowanie jak wcześniej. Funkcjonalny odbiór AAC06h zaliczony.
Stale0 nie testuje wymuszonego spóźnionego publish na S3; ten race sprawdzono
na hoście. Stop/join fault injection, długotrwała praca, MP3 z niezależnym
readerem i sieć pozostają osobno OPEN. WAIT w granicy EOF nie zastępuje
wymuszonego niedoboru w środku strumienia ani testu jitter/deadline.
Zapis tylko dokumentacji, bez powtarzania buildu/testów i bez flashowania.
Następny zakres: konfiguracja Wi-Fi według09 przed readerem HTTP/HTTPS.

## Etap09a — zabezpieczony SoftAP i prywatny build (2026-10-31)

- Nowy network worker: esp_wifi/esp_netif/esp_event z ESP-IDF6.1, AP WPA2,
  StreamOO-XXXXXX z MAC SoftAP, DHCP192.168.4.1/24, max4 klientów, kanał1.
  Priorytet3/stos6144B, log liczby klientów/stosu co10s. UI network snapshot
  pozostaje syntetyczny. Brak STA/portalu/mDNS/HTTP; to pierwszy test RF/audio.
- API i licencje zweryfikowane w lokalnym SDK (Apache2.0, w tym esp_wifi/lib),
  notice w licenses. Brak nowego pakietu, kopiowania kodu i zmian locka.
- Narzędzie getpass zapisuje wspólne hasło AP do zewnętrznego JSONv1 (0600,
  atomowo). CMake generuje prywatny header w ignorowanym build/. Bez pliku
  nie uruchamia Wi-Fi; jawny błędny plik blokuje konfigurację. Brak hasła
  zastępczego/logowania kluczy. WIFI_STORAGE_RAM, NVS init bez erase.
- Native obraz początkowo0x10dab0B przekroczył1MiB. Profil partycji zmieniony
  na gotowy SDK SINGLE_APP_LARGE1500KiB (0x177000); NVS0x9000/0x6000,
  PHY0xf000/0x1000, factory0x10000 bez zmian położenia. Potrzebny zwykły flash
  wraz z nową tablicą partycji, nie app-flash. OTA/LittleFS nadal OPEN.
- 179 testów hostowych PASS,11 nowych przypadków polityki AP i prywatnego
  procesu budowania, w tym redakcja błędów i zakaz sekretów w repo.
- Native build AAC+AP i MP3+AP z losowym kluczem testowym poza repo PASS;
  końcowy lokalny profil AAC bez sekretu AP (Wi-Fi wyłączone) PASS.
  To nie test docelowego hasła ani pracy radia na płytce. Testowy klucz nie
  trafił do repo ani logów; przed normalnym flash wymagany prywatny build.
- ADR0026 i NETWORK_BRINGUP. Odbiór09a OPEN: WPA2 association/zły klucz,
  DHCP/klienci, czyste AAC i stabilne PCM/heap przy RF. Nie flashowano,
  nie kasowano NVS i nie zapisywano sekretów użytkownika.

Końcowe build09a ESP-IDF6.1: AAC+AP0x10da90B, MP3+AP0x1145d0B PASS
przy factory0x177000B; wariant bez klucza AAC0x812b0B PASS. Końcowa
konfiguracja: sekret testowy usunięty, cache ścieżki pusty, header AVAILABLE0,
Wi-Fi nie uruchomi się przed lokalną konfiguracją użytkownika. Sprawdzono
brak testowego hasła w logach i końcowym obrazie. Walidator GPIO/platform,
diff --check PASS; callback render/pcm_fill nadal w IRAM.

## Log09a i nieczytelny fragment UART (2026-10-31)

Log użytkownika dla e369d49 potwierdza factory1500KiB, AP WPA2 i DHCP.
Klient dołączył w64.306s i otrzymał192.168.4.2 w64.396s; clients1 od71.266s.
Cycles1..25 po193536 ramek, stats_ok1/stale0, queued27..30/ACTIVE,
low0/hard0/starts1, invalid0, tap_drop/gaps0. max_fill133us, max_read39437us
od pierwszego cyklu (wall time pull w zadaniu, nie czas callbacku I2S).
Reader min stos1968B, audio_control22636B, network4096B.
Przed klientem internal90075B, po87319B (min49688, largest31744), stabilne
w próbkach70–100s; PSRAM8337172B stabilne. Nowa kalibracja RF przy pierwszym
starcie zapisana przez SDK; brak restartu/underrun w dostarczonym fragmencie.
Odsłuch przy RF, odrzucenie złego klucza i rozłączenie klienta nadal OPEN.

W80.776s pojedynczy wpis fixture_reader i następujące bajty są nieczytelne;
24 inne wpisy czytnika poprawne, przed i po tym ciągłość cykli zachowana.
Format źródłowy zawiera tylko stały ASCII i liczby (%PRIu32/%u z właściwymi
typami), konfiguracja UART0=115200, text log v1, PM/DFS wyłączone, stack canary
włączony. Brak dowodu błędu formatowania lub przepełnienia stosu. Wklejony
tekst zawiera znaki zastępcze Unicode, nie oryginalne bajty UART.
Użytkownik potwierdził „Dotykałem też przewodów”. Najbardziej prawdopodobne
przejściowe zakłócenie styku/transmisji UART; związek nieudowodniony samym
logiem. Nie wprowadzano pozornej poprawki firmware. Następna krótka próba:
stabilne TX/GND, bez dotykania przewodów. Jeśli objaw wróci, surowy zapis
bajtów i porównanie z natywną konsolą USB Serial/JTAG, jeśli jest dostępna;
secondary USB output już włączony. Zmiana wyłącznie dokumentacyjna.

## Zamknięcie obserwacji UART (2026-10-31)

Użytkownik powtórzył próbę i potwierdził: „Jest OK, to efekt poruszenia
przewodów przy dotykaniu enkodera, sprawdziłem jeszcze raz”. Przyczyna
nieczytelnego fragmentu konsoli potwierdzona jako połączenie UART poruszane
przy obsłudze enkodera. Obserwacja zamknięta; bez poprawki firmware i bez
powtarzania diagnostyki UART. Nie jest to osobne potwierdzenie odsłuchu
przy AP, próby złego klucza ani rozłączenia klienta. Zmiana dokumentacyjna.

## Etap09b — portal statusu i ekran danych AP (2026-10-31)

- Własny portal offline HTML/CSS/JS i GET/api/v1/status na natywnym
  esp_http_server6.1. HTTP task2/stos6144B,4 sockety/LRU, timeout3s.
  Tylko GET, bez admin mutations, CORS '*', zewnętrznych assetów i kluczy
  w DTO. Jawny schema_version1 i stały bufor JSON512B. Błąd snapshotu→503.
- Network owner publikuje kopię stanu bez klucza i hooks ready/stopped;
  adapter aplikacji łączy getter network/audio i start/stop HTTP poza IRQ.
  Portal nie zależy od implementacji ownerów. Błąd HTTP nie wyłącza AP/audio.
- Użytkownik potwierdził stałe/wbudowane hasło AP, SSID/hasło na OLED oraz
  docelowe AP off po STA/IP. Klucz nadal z istniejącego prywatnego buildu,
  stały runtime i wspólny dla urządzeń. Oddzielny ap_display tylko dla OLED,
  brak hasła w statusie HTTP/logach. Nie ustalono osobnego admin auth.
- UI AP ma pierwszeństwo, pełne63 znaki hasła42+21 bez ucięcia. VOL i
  PLAY/STOP nadal działają; lista/pressed-rotate nie otwierają się w tle.
  AP nieaktywne→HOME. Nie dodano nowego kodu/fontu zewnętrznego.
- 197 testów hostowych PASS: JSON/bounds, handlery i lifecycle z shimem HTTP,
  Node DOM/fetch oraz renderer OLED z pełnym kluczem/wyczyszczeniem ramek.
  ASan/UBSan4 przypadki handlerów/modelu PASS (bez LeakSanitizer).
  Chromium headless390px: lokalny przykładowy status, poprawny DOM/live JSON
  i screenshot zweryfikowane; to nie runtime esp_http_server na hoście/S3.
- API/licencja esp_http_server sprawdzone w lokalnym SDK6.1 (Apache2.0),
  bez nowych pakietów registry i bez kopiowania API WWW yoRadio. I2S,
  gain/tap/reader oraz NVS preferencji bez zmian. Partycja1500KiB bez zmian.
- ADR0027. Odbiór09b S3 OPEN: OLED/strona/status z rzeczywistym VOL/PLAY,
  brak klucza w HTTP/UART, stabilne PCM/heap i czysty odsłuch przy pollu.
  STA, AP off po uzyskaniu IP, zapis/rollback i admin auth nadal OPEN.
  Nie flashowano za użytkownika i nie zmieniano prywatnego klucza.

Build09b ESP-IDF6.1: AAC+AP/portal0x11b2c0B, MP3+AP/portal0x121e00B,
AP off0x814c0B PASS. Końcowy profil AAC+AP z istniejącą prywatną konfiguracją,
stress OFF/tap_delay0; factory1500KiB ma24% wolnej. Nie zmieniano klucza ani
zewnętrznego pliku. Walidator GPIO/platform i diff --check PASS; controlled_fill/
pcm_fill pozostają w IRAM. Nie uruchamiano buildu z ujawnianiem sekretów.

## Odbiór dźwięku09b i zmiana polityki AP (2026-10-31)

Użytkownik potwierdził „Dźwięk działa, ładnie”. Log a5ff0d9: cycles do29 po
193536 ramek, stats_ok1/stale0, low0/hard0/starts1, invalid0, max_fill133us,
max_read45026us (wall pull). Reader min stos1936B, audio_control22668B,
network3920B. Klient DHCP192.168.4.2 od57.433s; po wejściu klienta/strony
internal77463B (min39968), stabilne w próbkach80–110s, PSRAM8337004B.
Tap_drop/gaps wzrosły do7: dotyczy pomiarowego odgałęzienia, nie FIFO wyjścia,
zgodnie z izolacją tap. /favicon.ico404 jest brakującym opcjonalnym zasobem
przeglądarki, nie błędem audio. Odsłuch współpracy audio/AP/HTTP zaliczony;
wygląd OLED/strony nie został osobno opisany. Końcowy banner ROM nie zawiera
przyczyny resetu, nie przypisujemy go firmware bez dodatkowych danych.

Użytkownik ustalił, że w AP nie ma odtwarzania; przyszły wyjątek MicroSD jest
poza obecnym zakresem. Aplikacja nie uruchamia więc obu rodzajów testu audio
podczas bootu konfiguracji AP: bez dekodera/reader/tap/I2S, zamiast samego mute.
Enkoder w ekranie AP ignorowany, portal mówi Audio wyłączone w trybie AP.
NVS i klucz bez zmian. Bez AP dotychczasowe testy audio nadal dostępne.
202 testy hostowe PASS,5 nowych przypadków rzeczywistego startup main.c ze
stubami peryferiów. ADR0028. Odbiór nowej polityki AP na S3 OPEN.

Build polityki AP: ESP-IDF6.1 AAC+AP/portal0x11b370B i AP off0x81500B PASS,
factory1500KiB. Końcowy lokalny profil AP on, AAC wybrane jako test poza AP,
stress OFF/0, prywatny klucz i NVS bez zmian. Walidator GPIO/platform i
diff --check PASS.

## Rozszerzenie przyszłego zakresu — RC5 i telefon (2026-10-31)

Użytkownik zaplanował pilot RC5 i aplikację telefonu do sterowania przez API.
Wymagania zapisane w01/05/10, decyzje D25/D26 i ADR0029. Implementacja obu
funkcji OPEN: bez pinów/kodów pilota, wyboru bibliotek i technologii aplikacji.
Wspólne komendy/owner playera dla enkodera, RC5, WWW i telefonu; policy AP
bez odtwarzania obowiązuje wszystkich klientów. Nie dodano kodu ani nie
zmieniono kolejności bieżącego kroku (AP bez audio, potem STA). Weryfikacja
wyłącznie git diff --check, bez zbędnego powtarzania buildu/testów firmware.

## Kilka sieci Wi-Fi i prywatna playlista (2026-10-31)

Użytkownik wymaga zapisu wielu profili przez WWW i połączenia z pierwszą
dostępną siecią. Kolejność listy przyjęta jako priorytet; dalszy profil po
nieudanej próbie, AP off dopiero po STA/IP. Użytkownik wymaga uploadu stacji
z playlist.csv, dostarczonego lokalnie. Dodano /playlist.csv do .gitignore,
sprawdzono brak śledzenia w indeksie. Oryginał niezmieniony, bez kopii danych.
Zweryfikowano format UTF-8/TAB3/name-url-ovol względem prywatnego pliku i
referencji yoRadio; GPLv3 sprawdzone, kod/API upstream nieskopiowane.
Narzędzie read-only check_playlist.py raportuje kody/linie, nie dane stacji.
Audyt:267 rekordów,256 poprawnych,11 URL z U+200B,3 duplikaty URL jako
ostrzeżenia. Nie normalizowano pliku ani nie łączono się z URL stacji.
211 testów hostowych PASS,9 nowych testów syntetycznych. Nie zmieniono
firmware; bez nowego buildu/flash. ADR0030, D27/D28. WWW zapis profili/STA,
upload/preview/atomowy commit oraz admin auth nadal OPEN; nie przedstawiać
narzędzia hostowego jako działającego importera urządzenia.

## Etap 09c — zapis konfiguracji WWW (2026-10-31)

Formularze do 8 uporządkowanych sieci, autoryzacja kluczem AP, próby STA
z rollback i AP-off po IPv4. Import playlisty: preview → osobne zatwierdzenie
→ atomowy zapis LittleFS, bez prywatnej playlisty w repo. OLED pokazuje nowy
adres po wyłączeniu AP. Pełne decyzje i ograniczenia: ADR0031.

Weryfikacja: 221 testów hostowych PASS, Chromium 390px pełny przebieg formularzy
na syntetycznym backendzie PASS, node --check PASS, check_config --platform PASS,
native ESP-IDF6.1 build PASS: obraz 0x12dd60 B, 20% wolnej partycji aplikacji.
Parser firmware na RAM-kopii prywatnego CSV: 267 rekordów/11 normalizacji/3
duplikaty, oryginał niezmieniony. Nie wykonano flash/erase/eFuse.

HARDWARE_TEST_PENDING: zwykły pełny flash (nowa tablica partycji), Ctrl+F5,
import i reboot, kolejność/fallback sieci, AP-off i OLED/URL, heap/stosy.
Katalog jest przechowywany, ale syntetyczna lista OLED i odtwarzanie streamów
nie są jeszcze połączone z katalogiem. Audio pozostaje wyłączone w tym boot.

## Etap 09d — otwarty panel i HOME z IP (2026-10-31)

Użytkownik potwierdził działanie 09c; log dowodzi STA/IP i AP-off około 5 s
później. Heap internal po AP-off stabilny 142383 B, minimum 84512 B; network
stack HWM 4676 B. Osobny odbiór katalogu/restartu nie wynika z tego logu.

Na jawne polecenie usunięto dodatkowe logowanie WWW i sprawdzanie klucza AP
w HTTP. Panel sam odczytuje ustawienia. Zapis nadal walidowany i chroniony
przed obcym Origin, bez zmiany hasła sieci AP. Po AP-off/STA HOME, aktywne
gesty, IP w trzeciej linii y24..31. Brak samoczynnego uruchamiania tonu;
stream player/katalog OLED nadal przyszłe. ADR0032.

222 testy hostowe PASS, raster HOME/IP i HTTP bez auth; Chromium 390px pełne
formularze bez logowania PASS; node --check i native ESP-IDF6.1 build PASS.
Nie wykonano flash/erase/eFuse. HARDWARE_TEST_PENDING: nowy panel po Ctrl+F5,
HOME/IP po STA, gesty enkodera, dostęp WWW pod adresem LAN.

## Etap 08a — katalog LittleFS → OLED (2026-10-31)

Użytkownik potwierdził: „nowy ekran działa” — odbiór HOME/IP z 09d zaliczony.
Nowy komponent stations, deterministyczne ID name/URL/wystąpienie, snapshoty
PSRAM z referencjami. Startup i commit importu publikują katalog bez blokowania
UI na flash. Lista OLED używa wgranych nazw, aktualizacja do 1 s bez restartu;
wybór pokazuje nazwę na HOME, zachowując IP. Pusta lista wskazuje import WWW.
Brak uruchamiania streamu; nie pokazujemy fikcyjnej wizualizacji audio. ADR0033.

226 testów hostowych PASS; store/controls również ASan/UBSan PASS (LSan off,
bez TSan). Native ESP-IDF6.1 build PASS. Parser/katalog prywatnego pliku tylko
w RAM: 267 rekordów/267 różnych ID, reorder stabilny; oryginał bez zmian.
check_config --platform i diff --check PASS. Nowych zależności zewnętrznych brak.
Brak flash/erase/eFuse. Odbiór OLED/listy/wyboru/live-import/heap/stosów OPEN.
Następnie: źródło HTTP/ICY i połączenie wyboru z ownerem playera. Trwałe ID
przy edycji nazwy/URL oraz zapamiętywanie ostatniego wyboru nadal OPEN.

## Etap 06i — pierwszy player HTTP/ICY (2026-10-31)

Użytkownik potwierdził listę i wybór z 08a: „Działa fajnie”. Nowy owner player
łączy wybór OLED z niezależnym readerem HTTP/HTTPS, ICY, dekoderem MP3/AAC-LC
i dotychczasowym PCM/gain/I2S. Komendy/stan asynchroniczne, STOP/PLAY/volume,
retry tej samej stacji, AP bez I/O audio. PSRAM256KiB, task-only resample
mono/stereo8..48kHz →48kHz; kodeki/formaty poza zakresem i jakość SRC OPEN.
Pełne granice, ownership i zależności: ADR0034.

247 testów hostowych PASS; targeted actor/HTTP/PCM ASan/UBSan PASS (LSan off,
bez TSan). SDK transport/RTOS/decoder/DMA w testach zastąpione atrapami:
nie jest to odsłuch ani dowód deadline na S3. Native ESP-IDF6.1 build PASS,
obraz0x15abe0 B, 8% wolnej partycji; ELF IRQ fill/pcm_fill w IRAM, ring/pipeline
w DRAM. TLS z pełnym CA bundle i weryfikacją, SNTP po STA; licencje/source CA
w THIRD_PARTY_NOTICES. Nie wykonano flash/erase/eFuse ani połączeń do stacji
z prywatnego CSV. Dane prywatne nie są dodawane do obrazu/repo.

HARDWARE_TEST_PENDING: HTTP MP3 jako pierwszy odsłuch, potem AAC-LC/HTTPS/ICY,
rate44,1k/48k, heap/stack_hwm/reader_hwm/max_fill/low/hard, szybkie zmiany,
STOP i rebuffer. Procedura HTTP_PLAYER_BRINGUP. Nie uznawać playera/projektu
za odebrany przed logiem i odsłuchem. DSP, HE-AAC/reszta kodeków, HLS/M3U/PLS,
sterowanie WWW/mobile/RC5, last ID NVS i docelowy SRC pozostają dalsze.

## Odbiór 06i — pierwsze stacje sieciowe (2026-10-31)

Użytkownik: „Działa, sprawdziłem kilka stacji, słychać dobrze”. Log firmware
z 740c327 potwierdza PLAYING przy rate44100 (gen3/8) i48000 (gen5), również
zapisy zmienianej głośności do NVS. Pierwszy odsłuch kilku stacji zaliczony.
W widocznych sesjach PCM=32, invalid=0, low=0, hard=0, starts=1, max_fill111–112us.
Stos player HWM minimum8704 B, reader6404 B; internal heap w odtwarzaniu
61139–61599 B, minimum39844 B, largest31744 B; PSRAM minimum8031908 B.
To ograniczony czas obserwacji, nie test wielogodzinnej stabilności.

Jedna próba gen7: HTTP200/ESP_FAIL, decoder0/vendor0, format22050/2/16;
retry gen8 wraca do PLAYING44100. Przyczyna OPEN — log nie rozróżnia
zmiany formatu, braku postępu i pozostałych guardów pumpa. Nie uznajemy
22,05kHz za nieobsługiwane (konwerter wspiera ten rate). Bez zgadywania kodeka
lub HE-AAC na podstawie tej linii. Nie było widocznych błędów I2S/underrun.

Oddzielne potwierdzenie MP3 vs AAC-LC, HTTP vs HTTPS/ICY, STOP bez trzasków,
zaniki/rebuffer, szybkie zmiany i dłuższy soak pozostają OPEN. Kolejny punkt:
precyzyjna diagnostyka pojedynczego retry/pozostały odbiór, potem DSP/kolejne
funkcje. Zmieniono tylko dokumentację; bez powtarzania build/testów kodu
(ostatnie247 testów i build PASS). diff --check PASS; brak flash/erase/eFuse.

## Etap 06j — diagnostyka retry live decoder (2026-10-31)

Nowy `stage`/`cause` oraz poprzedni i obserwowany format, ostatnie in/used/out/
needed i liczniki per sesja. Pierwsza przyczyna zatrzaskuje się do init; WAIT
nie oznacza awarii. Rozróżnione EOF, źródło/kontrakt, decoder/status/bounds/
no-progress, PCM format i zmiana formatu. Bez zgadywania przyczyny gen7.
Gain=0 przed UART, okno fade od aktualnego czasu; callback I2S i resampler
bez zmian. ADR0035.

248 testów hostowych PASS, w tym15 wektorów przyczyn i formatowanie obu logów
przez actor; live/actor ASan/UBSan PASS (LSan off, bez TSan). Native ESP-IDF6.1
build PASS, obraz0x15b0f0 B, 7% wolnego app. check_config --platform oraz
diff --check PASS. Bez flash/erase/eFuse. Przyczyna sprzętowego retry nadal
OPEN: potrzebne nowe `Stream failed` + `Decode diag` dla jednej gen.

## 09e — STA przed AP i IP/VOL/belka (2026-10-31)

Polecenie użytkownika wdrożone: boot STA-first, kolejność profili i FAST_SCAN,
AP/ekran konfiguracji dopiero po niepowodzeniu całej listy lub bez profili.
Nieudana zmiana WWW przywraca stare profile przed AP; istniejące AP gaśnie
5s po IP. IP po lewej jasność6, VOL/belka jasność15. ADR0036.

266 testów hostowych PASS, w tym12 scenariuszy ownera sieci i testy rastra
wszystkich101 poziomów. Native ESP-IDF6.1 build PASS, obraz0x15b530, 7% wolne.
Bez nowych zależności, zmiany partycji i ścieżki IRQ I2S; bez flash/erase/eFuse.
Odbiór RF/czasu startu i fizycznego OLED OPEN; procedura NETWORK_BRINGUP.

Nowy log użytkownika z06j pokazuje PLAYING44100/48000 oraz22050 (gen6),
więc22,05kHz nie jest wykluczone. W gen13/14/20 stage=http_end/cause=none,
HTTP0/decoder0/vendor0/format0: timeout transportu/nagłówków przed dekoderem.
Nie dowodzi to przyczyny starego gen7 ani awarii HTTPS. Widoczne także
dwa błędy ramki MP3, po których jest PLAYING48000. Dalszy soak nadal OPEN.

## Odbiór09e i etap07a — fundament pomiarów stereo (2026-10-31)

Użytkownik po c5cd59c: „jest dobrze, idziemy dalej”. Zmiana STA-first i
wiersz IP/VOL zaakceptowane; osobne pomiary czasu RF i scenariuszy wszystkich
niedostępnych sieci nie wynikają z tego krótkiego potwierdzenia.

Własny komponent levels: RMS i peak L/R z signed32 stereo48kHz, przed gain.
Okna bez nakładania, domyślnie1920 ramek/40ms. Q15 z jawną kwantyzacją,
sumy64bit, brak przepełnienia na ujemnej pełnej skali. Luki/duplikaty
sekwencji kasują niepełne okno i unieważniają stary wynik. Brak alokacji,
I/O, zależności zewnętrznych, RTOS i zmian callbacku I2S. ADR0037.

To fundament obliczeń, jeszcze bez zadania analizy i publikowania do OLED.
FFT/smoothing/peak-hold, mapowanie dB/VU i pomiary CPU pozostają OPEN.
Nowego firmware nie trzeba wgrywać dla tego kroku — obraz UI bez zmian.

Weryfikacja07a:275 testów hostowych PASS;9 testów levels także ASan/UBSan
PASS (LSan off). Native ESP-IDF6.1 build PASS; levels skompilowany dla S3,
nieużywany kod usuwany przez linker, obraz nadal0x15b530 (7% wolne).
check_config --platform i diff --check PASS. Bez flash/erase/eFuse.

## 07b — RMS/peak na OLED przez osobny task (2026-10-31)

Komponent metering dołączony do istniejącego tap przed gain. Internal ring4,
priority1/stack4096B, analiza40ms; snapshot generation/timestamp, expiry200ms.
STOP/retry/buffering/zmiana stacji nie publikują starych poziomów. Zamknięcie
I2S przed STOP taska, zwolnienie po ACK; awaria pomiarów nie blokuje audio.
Bargraph i analog mają prawdziwe poziomy; FFT nadal bez danych. ADR0038.

284 testy PASS, metering/actor/controls ASan/UBSan PASS (LSan off, bez TSan).
Native ESP-IDF6.1 build PASS:0x15bcd0, 7% app wolne; check_config --platform
i diff --check PASS. Pcm_fill w IRAM; bez obliczeń/alokacji/I/O w callbacku.
Bez nowych zależności zewnętrznych, zmiany partycji, flash/erase/eFuse.
HARDWARE_TEST_PENDING: DSP_BRINGUP — reakcja obu mierników, VOL0/50/100,
STOP/zmiany stacji, heap/stack/max_fill/low/hard i nowe logi meters.
Rzeczywisty koszt kopii tap i CPU/DMA/SMP wymaga urządzenia. Smoothing,
peak-hold, skala dB/VU i FFT to dalsze kroki.

## 07c — wygładzanie mierników i miganie (2026-10-31)

Użytkownik potwierdził07b, zgłosił zbyt szybki ruch i migające „Brak danych”.
Log PLAYING44100/PCM32, invalid/low/hard0, max_fill133us. gaps23/drop28
wyjaśniają reset ważności między oknami; nie dowodzą przerwy audio.
HWM metering min3280B, heap internal min35136B, max_work14252us.

Poziomy prezentowane10Hz, RMS attack160/release500ms, peak hold400/release800ms.
Ostatni kompletny pomiar zachowany po krótkiej luce, bez odświeżania timestampu
przez samą lukę; expiry200ms oraz STOP/generation bez zmian. ADR0039.
286 testów PASS; ballistics/metering ASan/UBSan PASS (LSan off).
Bez zmian audio/ISR, nowych zależności, flash/erase/eFuse. Odbiór OLED OPEN.

Native ESP-IDF6.1 build PASS, obraz0x15bf20 (7% wolne);
check_config --platform i diff --check PASS. Bez flash/erase/eFuse.

## 07c-2 — żywsza reakcja mierników (2026-10-31)

Użytkownik: działa, ale za mało ruchu/zbyt uśrednione. Zmniejszono wyłącznie
wygładzanie: RMS attack30/release150ms, peak hold150/release300ms. Takt10Hz
i ochrona przed miganiem bez zmian. ADR0040. Test skoku potwierdza ponad75%
reakcji po pierwszych100ms; po spadku do0 mniej niż40% po200ms.
Odbiór widocznego ruchu na OLED OPEN. Bez flash/erase/eFuse.

Weryfikacja:286 testów PASS, ballistics/metering ASan/UBSan PASS (LSan off).
Native ESP-IDF6.1 build PASS,0x15bf20 (7% wolne).
check_config --platform i diff --check PASS.

## 07d — referencyjny rdzeń FFT (2026-10-31)

Użytkownik odkłada strojenie RMS/peak. Parametry z e8f96f2 bez zmian.
Własny spectrum: float radix2 N1024/2048, Hann N-1, normalizacja amplitudy,
stereo w domenie mocy bez znoszenia faz, 24 pasma bez dziur/pustych zakresów.
Sam rdzeń, jeszcze bez kolektora overlap/live/OLED. ADR0041.
Audyt oficjalnego ESP-DSP1.8.2/API/licencji/manifestu pod przyszły backend;
nie dodano zależności ani kodu stron trzecich. Porównanie FFT sc16/wydajności
i build przyszłego adaptera OPEN. NumPy1.26.4 użyte jako oracle hostowy.

Weryfikacja07d:295 testów hostowych PASS (oracle NumPy wykonany, bez skip).
Native C FFT/PCM harness ASan/UBSan PASS, LSan off. Native ESP-IDF6.1 build
PASS, obraz0x15bf20 (7% wolne); spectrum kompilowany, nieużywany kod
usunięty przez linker. check_config --platform i diff --check PASS.
Bez flash/erase/eFuse i zmian partycji.

## 07e — FFT na OLED z rzeczywistych próbek (2026-10-31)

Kolektor1024/50% overlap i reset po lukach. Widmo24 pasma -60..0dBFS,
max10Hz, tylko aktywne FFT/HOME/PLAYING. Workspace około45KiB PSRAM,
niezmieniony task i ring internal. Błąd PSRAM nie zatrzymuje audio/RMS.
Rewizja żądania i generation chronią przed starymi pasmami, niezależna
ważność/czas FFT i RMS, expiry200ms. Parametry mierników bez zmian.
Referencyjny float backend jako pierwszy pomiar live; optymalizacja
ESP-DSP/SIMD/sc16 nadal OPEN. ADR0042.

303 testy PASS, NumPy PASS, collector/metering/actor/controls ASan/UBSan
PASS (LSan off, bez TSan). Native ESP-IDF6.1 build PASS,0x15f2d0 (6% wolne).
check_config --platform/diff --check PASS, pcm_fill w IRAM. Bez nowych
zależności, zmian partycji, flash/erase/eFuse.
HARDWARE_TEST_PENDING: DSP_BRINGUP — słupki FFT, czysty dźwięk, VOL0,
zmiana widżetu/STOP/stacji, max_fft/max_fill/drop/heap/stack.

## 07f — widmo: DC ważone Hann i łagodne opadanie (2026-10-31)

Użytkownik: przy mowie słupki migają/znikają, zostaje pusta linia bez
„Brak danych”. Hann stosowane; test off-bin ujawnił artefakt dolnego pasma
ze zwykłej średniej przed oknem. DC removal zmienione na ważone Hann,
NumPy oracle zaktualizowane i test przecieku względem prostokąta PASS.
FFT envelope: natychmiastowy atak, opadanie pełnej skali320ms, bez
uśredniania FFT/RMS i bez zmiany ich kalibracji. ADR0043.

305 testów PASS; envelope/metering/native FFT ASan/UBSan PASS (LSan off).
Native ESP-IDF6.1 build PASS,0x15f3e0 (6% wolne); check_config/diff PASS.
Brak nowych zależności, alokacji, zmian ISR/audio/partycji i flash/erase/eFuse.
Odbiór OLED/nowych czasów max_fft na urządzeniu OPEN.

## Odbiór07f i dane techniczne HOME (2026-10-31)

Użytkownik potwierdził: dźwięk OK, słupki lepsze; strojenie widma/RMS
zostaje do późniejszego dopracowania. Nowych czasów max_fft nie podano,
więc wydajność/SIMD/soak nadal OPEN.

ADR0044: rzeczywisty kodek MP3/AAC po znanym formacie PCM, czyszczenie
przy STOP/retry/braku sieci; RSSI aktualizowane co1s w ownerze sieci,
ikona/dBm w HOME. Brak bitrate pozostaje --, bez estymacji. Renderer
nie wykonuje I/O; FFT/RMS/geometria/audio bez zmian.

307 testów hostowych PASS; network/actor/controls ASan/UBSan PASS
(LSan off). Native ESP-IDF6.1 build PASS,0x15f690 (6% wolne),
check_config --platform/diff --check PASS. Bez nowych zależności,
flash/erase/eFuse/partycji. Odbiór nowego panelu na OLED OPEN.

## Host: test playlisty/kodeków/bitrate (2026-10-31)

Użytkownik widzi MP3 na sprawdzonych stacjach, AAC nie znalazł. Dodano
Python probe_playlist: dostępność z retries/timeouts, codec/profile/bitrate
z pakietów ffprobe, tabela i JSON, osobna active.csv. Oryginał zachowany,
kolejność/ovol/duplikaty zachowane, prywatne wyjścia ignorowane. ADR0045.
314 testów hostowych PASS,7 nowych; realny loopback HTTP/ffprobe6.1.1:
MP3 128kb/s, AAC-LC około73.5kb/s,404 rozpoznane. Native incremental build
PASS, firmware bez zmian. diff --check PASS; brak flash/erase/eFuse.
Nie odpytano prywatnej playlisty; wyniki dostępności OPEN. Instrukcja
PLAYLIST_PROBE.md. Host ACTIVE nie gwarantuje obsługi kodeka przez firmware.

## HOME — odstęp RSSI (2026-10-31)

ADR0046: RSSI przesunięte6px w lewo, ikona zwężona, odstęp przed wizualizacją.
Również -127 mieści się bez kolizji. Golden frames zaktualizowane po
sprawdzeniu podglądu; różnice tylko w lewym dolnym panelu. Native build
PASS (0x15f690,6% wolne), bez flash/erase/eFuse. Odbiór OLED OPEN.

Pełne314 testów hostowych PASS, diff --check PASS.

## Zegar SNTP i strefa WWW (2026-10-31)

Użytkownik wymaga wyboru strefy w Konfiguracja WWW, z automatycznym DST.
ADR0047: rzeczywista data/czas na OLED po SNTP, przedtem kreski; UTC
systemowe/TLS bez zmiany.8 stref UTC/europejskich, defaultWarsaw, NVS
string/cached getter/zapis dopiero po udanym commit. Nowe zakładki WWW
Status/Konfiguracja, formularz Data i czas, GET/POST /api/v1/clock.

328 testów PASS, porównanie zoneinfo2020..2099 i granic DST dla8 stref,
NVS failures/noop/restore, SNTP callback, HTTP i UI. Clock-settings/network/
controls ASan/UBSan PASS (LSan off), node --check i Chromium390px PASS
(tabs/save/bez overflow oraz poprzednie formularze). Native ESP-IDF6.1
build PASS,0x160ff0 (6% wolne), config/diff PASS, pcm_fill w IRAM.
Bez nowych zewnętrznych zależności, zmian audio/DSP/partycji i flash/erase/eFuse.
HARDWARE_TEST_PENDING: CLOCK_BRINGUP — NTP, godzina, zmiana strefy i reboot.

## Data DMY i zapis strefy (2026-10-31)

Użytkownik potwierdza zegar i zmianę strefy. Data zmieniona na DD.MM.YYYY,
nieznana --.--.----; geometria i czas bez zmian. NVS zapis już działał,
dodano test zapisu/odczytu w świeżym procesie z plikową atrapą NVS, PASS.
Clock-settings ASan/UBSan PASS (LSan off). ADR0048.
Golden frames po podglądzie, zmiany ograniczone do panelu daty.
Nowy format i fizyczny power-cycle strefy do odbioru. Brak flash/erase/eFuse.

Weryfikacja:329 testów hostowych PASS, clock-settings ASan/UBSan PASS
(LSan off), native ESP-IDF6.1 build PASS,0x160ff0 (6% wolne),
check_config --platform i diff --check PASS. Bez flash/erase/eFuse.

## Bitrate strumienia — deklaracja ICY (2026-10-31)

Użytkownik fizyczny test strefy po power-cycle wykona w międzyczasie;
nie traktujemy go jako zaliczonego. Kontynuacja ADR0049: icy-br1..4096
z HTTP do playera/UI, błędne opcjonalne dane bez wpływu na audio, reset
na redirect i przy STOP/retry/offline. Bez nagłówka --; bez udawania
pomiaru VBR. Native tor audio/FFT i ustawienia NVS bez zmian.
332 testy PASS; HTTP/actor ASan/UBSan PASS (LSan off). Sprzętowy bitrate
i data po zmianie formatu oraz power-cycle strefy pozostają OPEN.

Native ESP-IDF6.1 build PASS,0x161080 (6% wolne), GPIO validator
i diff --check PASS. Bez nowych zależności, flash/erase/eFuse i zmian partycji.

## Sterowanie WWW playerem (2026-10-31)

Użytkownik: wszystko działa, kontynuować. Odbiór bieżącego panelu bitrate
potwierdzony ogólnie; fizyczny test NVS strefy nadal niezależny/OPEN.
ADR0050: Status WWW z PLAY/STOP/volume, POST /api/v1/player/control,
explicit idempotent commands i absolute volume w kolejce jednego ownera
z encoderem. Blokada AP/offline, PLAY tylko wybrana stacja. Bez zmian
DSP/ISR/audio ramp i zapisów preferencji.
334 testy PASS, actor ASan/UBSan PASS (LSan off), node --check i Chromium
390px PASS (AP-disabled, LAN slider/STOP/PLAY/status oraz stare formularze).
Native ESP-IDF6.1 build PASS,0x161f00 (6% wolne), config/diff PASS.
Bez nowych zależności, partycji, flash/erase/eFuse. Odbiór sprzętowy
WEB_PLAYER_BRINGUP OPEN; wybór stacji WWW nadal następny zakres.

## WWW AP/LAN i edytor playlisty (2026-10-31)

Polecenie użytkownika: AP tylko Wi-Fi/import/zegar, LAN player i lista
klikanych stacji oraz edycja/add/delete/export/import/clear. Wdrożono
ADR0051, serwerowe gate STA/AP-off dla nowych czterech endpointów,
snapshoty, CAS edycji, JSON IDs/generation jako stringi i trwały pusty CSV.
Player/OLED wspólne źródło i właściciel. ovol/order/duplikaty zachowane.
334 testy PASS (rozszerzone istniejące scenariusze), store/actor ASan/UBSan
PASS (LSan off), pełny Chromium390px AP/LAN/editor/conflict PASS. Node,
validator i diff PASS. Native ESP-IDF6.1 build PASS,0x165fa0 (5% wolne).
Bez zmiany partycji/audio/DSP/ISR, flash/erase/eFuse i danych prywatnych.
Odbiór urządzenia OPEN: WEB_LIBRARY_BRINGUP. ID niezależne od edycji nazwy/
URL i fizyczny test trwałości strefy pozostają OPEN.

Dodatkowo rzeczywiste nowe handlery portal configuration przeszły ASan/UBSan
(LSan off), obejmując listę/eksport/select/replace i blokady AP.
