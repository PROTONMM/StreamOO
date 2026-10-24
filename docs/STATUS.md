# Status implementacji

| Etap | Stan | Dowód |
|---|---|---|
| 01 zakres | wymagania zebrane | dokumentacja |
| 02 schemat i profil | walidator i menuconfig zaimplementowane; HARDWARE_TEST_PENDING | ADR 0001; 5 testów hostowych, 2332 konfiguracje; build PASS |
| 03 szkielet | baza zaimplementowana; odbiór sprzętowy i integracyjny OPEN | ESP-IDF v6.1 build PASS; ADR 0002 |
| 04 UI/SSD1322 | HOME i lista odebrane na OLED; końcowe strojenie fontu OPEN | ADR 0012; odbiór użytkownika |
| 05 encoder | dekoder/IRQ i gesty odebrane; interaktywny adapter odebrany w trybie A; preferencje NVS odebrane; shutdown/flush OPEN | ADR 0013–0017; 99 testów PASS |
| 06 audio | natywne wyjście I2S i skończony test kanałów; HARDWARE_TEST_PENDING | ADR 0018; 107 testów PASS |
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
