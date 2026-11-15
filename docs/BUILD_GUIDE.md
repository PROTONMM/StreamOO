# StreamOO — menuconfig, budowanie i wgrywanie

[English version](en/BUILD_GUIDE.md)

Stan instrukcji: 2026-11-14. Profil: ESP32-S3-WROOM-1-N16R8, SSD1322,
PCM5102A, natywny ESP-IDF **6.1**. Obsługa gotowego radia:
[USER_GUIDE.md](USER_GUIDE.md). Polecenia poniżej wykonuj z katalogu głównego
projektu w terminalu z aktywnym środowiskiem ESP-IDF, chyba że podano inaczej.

## Przygotowanie środowiska

Zainstaluj ESP-IDF **6.1** wraz z narzędziami dla ESP32-S3. Oficjalna
[instrukcja Espressif dla v6.1](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32s3/get-started/index.html)
prowadzi przez ESP-IDF Installation Manager (EIM) na Linux, Windows i macOS.
Projekt sprawdza wersję 6.1 i target `esp32s3` w CMake; inne wersje SDK
nie są przyjmowane przez tę kontrolę.

Na Linux/macOS aktywuj zainstalowane środowisko w każdym nowym terminalu.
Jeśli SDK znajduje się w poniższym katalogu, polecenia wyglądają tak:

```sh
source "$HOME/.espressif/v6.1/esp-idf/export.sh"
idf.py --version
```

Dostosuj ścieżkę do swojej instalacji lub użyj skryptu aktywacyjnego EIM.
Na Windows otwórz terminal ESP-IDF skonfigurowany przez instalator; zamiast
`python3` używaj `python`, jeśli tak nazywa się interpreter tego środowiska.
Przykłady wielowierszowych komend z `\` są dla powłoki Linux/macOS;
na Windows wpisz daną komendę w jednym wierszu.

Potrzebne są Git, Python ze środowiska IDF, CMake, Ninja i toolchain Xtensa
dostarczony z SDK. Pierwsza konfiguracja może pobrać komponenty z rejestru,
więc wymaga dostępu do Internetu. Wersje zależności określają pliki
`idf_component.yml` w komponentach i `dependencies.lock`; nie aktualizuj
ich przy okazji zwykłego budowania.

Bieżąca implementacja znajduje się na `main`; PR #1 został scalony.
Tag `v0.1.0` wskazuje pierwsze wydanie źródeł. Aby uzyskać checkout:

```sh
git clone --branch main https://github.com/PROTONMM/StreamOO.git
cd StreamOO
```

Dla odtworzenia konkretnego wydania użyj `git switch --detach v0.1.0`.
Poprawki po wydaniu znajdują się na `main` dopiero po scaleniu.
Wskazówki dla współtwórców: [CONTRIBUTING.md](../CONTRIBUTING.md).

## Gdzie zmienia się ustawienia

| Rodzaj ustawienia | Miejsce | Potrzebny nowy build? |
| --- | --- | --- |
| GPIO, tryb enkodera A/B, bufor, FFT, dekoder FLAC, diagnostyka | `idf.py menuconfig` → StreamOO Configuration | Tak |
| Wspólne hasło punktu konfiguracji AP | Prywatny plik tworzony przez `tools/configure_network.py` | Tak |
| Domowe sieci Wi-Fi, playlista, strefa, autoplay, nazwa `.local`, czas listy | Panel WWW radia | Nie |
| Głośność i wizualizacja | Enkoder; głośność także WWW | Nie |
| Motyw WWW | Nagłówek strony; pamięć przeglądarki | Nie |

`sdkconfig.defaults` jest zestawem ustawień początkowych. `sdkconfig` to
wygenerowana lokalna konfiguracja i **ma pierwszeństwo** przed defaults.
Zmiana defaults ani pobranie nowej wersji projektu nie nadpisują wszystkich
wcześniejszych wyborów. `menuconfig` zapisuje zmiany w `sdkconfig`, nie
w urządzeniu; zaczną działać po kompilacji i flashowaniu.

## Prywatne hasło AP

Przed pierwszym buildem przeznaczonym do pracy z Wi-Fi uruchom:

```sh
python3 tools/configure_network.py
python3 tools/configure_network.py --check
```

Skrypt poprosi dwukrotnie o hasło, bez wyświetlania wpisywanych znaków.
Wymaga 8–63 drukowalnych znaków ASCII. Używaj tego samego ustalonego hasła
AP przy budowaniu dla kolejnych urządzeń; SSID będzie różne dzięki MAC.

Plik jest zapisywany poza repozytorium:
`~/.config/streamoo/network.json`, albo pod `$XDG_CONFIG_HOME/streamoo/`,
jeśli ta zmienna jest ustawiona. Nie podawaj hasła w argumentach polecenia,
`sdkconfig` ani w plikach defaults. Skrypt odrzuca lokalizację wewnątrz repo,
także ignorowany katalog `secrets/`.

Po pierwszym utworzeniu pliku wykonaj `idf.py reconfigure build`.
Przy kolejnych buildach nie musisz ponownie wpisywać hasła.
Brak domyślnego pliku pozwala skompilować firmware, lecz **sieć pozostaje
wyłączona**, również przy `STREAMOO_NETWORK_ENABLED=y`. Nie ma zastępczego
otwartego AP. Błędny plik wskazany jawnie przerywa konfigurację.

Opcjonalnie wybierz inną lokalizację poza checkoutem:

```sh
python3 tools/configure_network.py --file /sciezka/poza/repo/network.json
idf.py -DSTREAMOO_NETWORK_SECRETS_FILE=/sciezka/poza/repo/network.json reconfigure build
```

Zastąp przykładową ścieżkę rzeczywistą. Wybór zostaje w cache CMake danego
katalogu budowania. Powrót do domyślnego pliku:
`idf.py -DSTREAMOO_NETWORK_SECRETS_FILE= reconfigure build`.
Hasło trafia do prywatnego nagłówka w `build/` i do binarki firmware;
nie publikuj artefaktów zawierających prywatny klucz jako neutralnych przykładów.

## Pierwsza konfiguracja i menuconfig

W nowym checkoutcie bez własnej konfiguracji:

```sh
idf.py set-target esp32s3
idf.py menuconfig
```

W istniejącym projekcie z poprawnym `sdkconfig` wystarcza:

```sh
idf.py menuconfig
```

`set-target` czyści katalog budowania i odtwarza konfigurację, zachowując
poprzednią jako `sdkconfig.old`; nie uruchamiaj go przy każdej aktualizacji.
Znaczenie komend opisuje [oficjalna dokumentacja idf.py](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32s3/api-guides/tools/idf-py.html).

W menu wybierz **StreamOO Configuration**. Strzałki i Enter służą do
nawigacji, Spacja do przełączania opcji, `/` do wyszukiwania symbolu,
`?` do pomocy. Zapisz przez **Save**, potem wyjdź przez **Exit**.
Przy wyszukiwaniu można wpisać np. `STREAMOO_ENCODER_MODE` lub
`STREAMOO_FLAC_DR`. Opcje zależne pojawią się po włączeniu ich warunków.

### Normalny profil radia

| Opcja w StreamOO Configuration | Symbol `CONFIG_…` | Zwykły profil |
| --- | --- | --- |
| Enable network radio and Wi-Fi configuration | `STREAMOO_NETWORK_ENABLED` | Włączone |
| Enable encoder input | `STREAMOO_ENCODER_ENABLED` | Włączone |
| Enable interactive radio UI | `STREAMOO_UI_ENABLED` | Włączone; wymaga enkodera i ekranu |
| Hardware: SSD1322 → Enable SSD1322 display | `STREAMOO_DISPLAY_ENABLED` | Włączone |
| Reserve GPIO19/20 for USB debugging | `STREAMOO_RESERVE_USB` | Włączone |
| Prepare hardware AES/SHA before I2S | `STREAMOO_CRYPTO_EARLY_INIT` | Włączone przy sprzętowym AES/SHA |
| Compile detailed UART diagnostics and runtime profiling | `STREAMOO_DIAGNOSTICS` | Wyłączone |
| Compile hardware test sources and synthetic screens | `STREAMOO_HARDWARE_TESTS` | Wyłączone |

Przygotowanie crypto przed I²S jest częścią zwykłego profilu; nie wyłączaj
go jako rzekomo zbędnego testu. Ustawienia SDK spoza menu StreamOO, zapisane
w defaults: flash 16 MB, Octal PSRAM 80 MHz z testem pamięci i alokacją przez
malloc, CPU 240 MHz, własna tablica `partitions.csv`, optymalizacja rozmiaru,
sprzętowe AES/SHA, okno TCP 23040 B i mailbox 18. Normalne logowanie: WARN,
z wybranymi informacjami aplikacji. Walidator kontroluje profil N16R8 i GPIO.

### Hardware: SSD1322

| Pole | Symbol `CONFIG_STREAMOO_…` | Wartość początkowa |
| --- | --- | --- |
| SPI host | `LCD_SPI_HOST` | 2 (SPI2), alternatywnie 3 |
| MOSI GPIO | `LCD_MOSI` | 11 |
| SCLK GPIO | `LCD_SCLK` | 12 |
| CS GPIO | `LCD_CS` | 9 |
| DC GPIO | `LCD_DC` | 10 |
| RESET GPIO | `LCD_RST` | −1: bez resetu sterowanego GPIO |
| SPI clock (MHz) | `LCD_SPI_CLOCK_MHZ` | 10; zakres 1–10 |
| Experimental panel column offset | `LCD_COLUMN_OFFSET` | 28; jednostka to 4 piksele |
| Experimental A0 remap byte (decimal) | `LCD_REMAP` | 20, czyli 0x14 |

Profil odpowiada używanemu panelowi 256×64 i interfejsowi 4-wire SPI.
Offset/remap zmieniaj tylko przy dopasowaniu innego panelu i kontroli obrazu.
RESET=−1 wymaga zapewnienia resetu przez sprzęt. Szczegóły połączeń:
[02_HARDWARE.md](02_HARDWARE.md).

### Hardware: PCM5102A (I2S)

| Pole | Symbol `CONFIG_STREAMOO_…` | GPIO |
| --- | --- | --- |
| I2S DATA OUT GPIO | `I2S_DOUT` | 48 |
| I2S BCLK GPIO | `I2S_BCLK` | 47 |
| I2S LRCLK GPIO | `I2S_LRCLK` | 21 |

Nie jest używany osobny MCLK. Testy tonu i źródeł plikowych w tym menu
pozostaw wyłączone przy zwykłym radiu.

### Hardware: Encoder

| Pole | Symbol `CONFIG_STREAMOO_…` | Wartość |
| --- | --- | --- |
| Encoder A GPIO | `ENCODER_GPIO_A` | 5 |
| Encoder B GPIO | `ENCODER_GPIO_B` | **6** |
| Encoder button GPIO | `ENCODER_GPIO_BUTTON` | 7 |
| Enable internal pull-ups | `ENCODER_INTERNAL_PULLUP` | Wyłączone w używanym sprzęcie |
| Invert encoder direction | `ENCODER_REVERSED` | Włączone |
| Use half-quadrature detents | `ENCODER_HALF_QUADRATURE` | Włączone |
| Encoder interaction mode | `ENCODER_MODE_A` / `ENCODER_MODE_B` | A |

Tryb A: krótki klik otwiera listę podczas grania lub uruchamia PLAY po STOP;
długi zatrzymuje. Tryb B: krótki PLAY/STOP, długi lista.
Pełna tabela gestów: [instrukcja obsługi](USER_GUIDE.md#enkoder).

Domyślne **5/6/7** odpowiadają potwierdzonym połączeniom użytkownika.
Historyczny `config/yr_031.defaults` odzwierciedla schemat z **B=16**;
nie nakładaj go bez sprawdzenia swojej płytki. Przy odwróconym kierunku
zmień `Invert encoder direction`; podciąganie i liczbę kroków dobierz
do rzeczywistego enkodera.

### Audio and DSP

| Pole | Symbol `CONFIG_STREAMOO_…` | Zwykły profil / zakres |
| --- | --- | --- |
| FLAC decoder | `FLAC_DR` / `FLAC_MICRO` / `FLAC_ESPRESSIF` | dr_flac 0.13.4 |
| Compressed stream ring size (KiB) | `STREAM_BUFFER_KIB` | 256; zakres 64–2048 |
| FFT size | `FFT_1024` / `FFT_2048` | 1024; opcjonalnie 2048 |
| Use ESP-DSP sc16 SIMD FFT on ESP32-S3 | `DSP_ENABLE_SIMD` | Włączone |

Większy bufor zużywa więcej PSRAM; sama zmiana pojemności nie gwarantuje
usunięcia przerw. FFT 2048 zwiększa koszt analizy. Zmiany wymagają kompilacji
i próby odsłuchowej. Wybór wizualizacji pozostaje gestem enkodera, nie opcją
rozmiaru FFT. Alternatywne backendy FLAC służą także porównaniom; mają inne
ograniczenia i historię odbioru: [FLAC_BACKENDS_BRINGUP.md](FLAC_BACKENDS_BRINGUP.md).

### Walidacja GPIO

Po zapisaniu `menuconfig`:

```sh
python3 tools/check_config.py sdkconfig --platform
```

Oczekiwane: `N16R8 GPIO map check passed`. CMake wykonuje tę kontrolę również
automatycznie. Odrzucane są kolizje, nieistniejące GPIO 22–25,
zarezerwowane dla pamięci 26–37 oraz strapping 0/3/45/46. GPIO 19/20 są
chronione przy rezerwacji lub użyciu USB; 43/44 przy domyślnej konsoli UART.
Nie obchodź błędu walidatora przez wyłączenie kontroli. Dostępność na złączu
trzeba dodatkowo sprawdzić w schemacie konkretnej płytki.

## Budowanie

```sh
idf.py reconfigure build
```

Przy kolejnych zmianach kodu wystarcza `idf.py build`. Po pierwszym utworzeniu
prywatnego pliku lub zmianach zmiennych CMake użyj ponownie `reconfigure`.

Wynik znajduje się w ignorowanym katalogu `build/`:

| Plik | Znaczenie |
| --- | --- |
| `streamoo.bin` | Aplikacja firmware |
| `streamoo.elf` | Symbole potrzebne m.in. do analizy awarii |
| `bootloader/bootloader.bin` | Bootloader |
| `partition_table/partition-table.bin` | Tablica partycji |
| `flash_args` | Argumenty wgrywania dla tego buildu |
| `config/sdkconfig.h` | Faktyczna wygenerowana konfiguracja kompilacji |

Do oceny rozmiaru użyj `idf.py size` lub `idf.py size-components`.
Nie trzeba generować osobnego obrazu LittleFS: katalog stacji zapisuje radio
po imporcie WWW, a pliki panelu są osadzone w aplikacji.

## Wgrywanie i aktualizacja

Podłącz płytkę kablem danych i wybierz rzeczywisty port, np.
`/dev/ttyUSB0` albo `/dev/ttyACM0` w Linux, `/dev/cu.usbmodem…` na macOS,
`COM4` na Windows. Zamknij inne programy korzystające z tego portu.

```sh
idf.py -p /dev/ttyUSB0 flash monitor
```

Polecenie zbuduje potrzebne pliki i wgra bootloader, tablicę partycji oraz
aplikację. Wyjście z monitora: **Ctrl+]**. Sam podgląd konsoli:
`idf.py -p /dev/ttyUSB0 monitor`. Na Windows zastąp port przez np. `COM4`.
Jeżeli automatyczne wejście w bootloader nie działa, użyj sekwencji
BOOT/RESET właściwej dla płytki i ponów flash.

Aktualny układ [partitions.csv](../partitions.csv):

| Partycja | Offset | Rozmiar |
| --- | --- | --- |
| NVS | `0x9000` | `0x6000` |
| PHY | `0xf000` | `0x1000` |
| factory (aplikacja) | `0x10000` | `0xEF0000` |
| storage (LittleFS) | `0xF00000` | `0x100000` |

To jedna duża aplikacja, **bez OTA i slotu rollback**. Zwykły `flash`
przy tym samym układzie nie wgrywa obrazu NVS/LittleFS i zachowuje ustawienia
oraz stacje. Nie używaj `erase-flash` do zwykłej aktualizacji. Przed migracją
z historycznego układu partycji wyeksportuj playlistę; po przeniesieniu
LittleFS potrzebny może być import. Przy pierwszym wgraniu i zmianie tablicy
użyj pełnego zestawu `flash`, a nie samego `app-flash`.

Po restarcie sprawdź Wi-Fi, jedną znaną stację, PLAY/STOP, głośność, ekran
i panel WWW. Odśwież stronę po aktualizacji. Instrukcja pierwszej konfiguracji:
[USER_GUIDE.md](USER_GUIDE.md#pierwsze-uruchomienie).

## Diagnostyka i testy

Testy hostowe uruchomione na komputerze nie flashują urządzenia:

```sh
python3 -m unittest discover -s tests -v
```

Potrzebne są hostowy kompilator C oraz Python; część testów frontendu
korzysta z Node.js. NumPy umożliwia porównanie FFT z niezależnym wzorcem;
brak opcjonalnego narzędzia może spowodować SKIP, a nie zaliczenie testu.
Testy przeglądarkowe wymagają dodatkowo lokalnych socketów oraz Chrome
pod ścieżką `/usr/bin/google-chrome` używaną obecnie przez te skrypty:

```sh
python3 tools/check_portal_browser.py
python3 tools/check_portal_layout.py
```

Te dwa skrypty używają syntetycznych danych i lokalnego serwera; nie
sprawdzają fizycznego telefonu, radia ani dostępności stacji.

Do logów rzeczywistego radia włącz `Compile detailed UART diagnostics and
runtime profiling`. Na długie sesje wybierz w ustawieniach FreeRTOS licznik
runtime U64 i zegar esp_timer. Alternatywnie zbuduj osobny profil:

```sh
idf.py -B build/diagnostics \
  -D SDKCONFIG=sdkconfig.diagnostics.local.defaults \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.diagnostics.defaults' build
```

To nowy, prywatny pełny `sdkconfig`, mimo końcówki `.local.defaults`; jest
ignorowany przez Git. Overlay działa przy jego pierwszym tworzeniu.
Własne zmiany GPIO z głównego `sdkconfig` nie kopiują się automatycznie —
przed flash sprawdź ten profil. Edytuj go tym samym poleceniem, zastępując
`build` przez `menuconfig`. Wgrywając diagnostykę zachowaj te same `-B` i `-D`,
zastąp końcową akcję przez `-p PORT flash monitor`. Samo `-B` nie wybiera
osobnej konfiguracji. Powrót do zwykłego obrazu: standardowy build i flash
z głównego katalogu `build/`.

`Compile hardware test sources and synthetic screens` włącza dodatkowo
diagnostykę i udostępnia opcje testów. Samo zaznaczenie nie uruchamia tonu.

| Próba | Ustawienia w StreamOO Configuration |
| --- | --- |
| Plik AAC/MP3 lub sinus sterowany enkoderem | `HARDWARE_TESTS=y`, `NETWORK_ENABLED=n`, `UI_ENABLED=y`, `AUDIO_CONTROL_TEST=y`; wybierz źródło w menu I2S |
| Skończony test OLED i kanałów I2S | `HARDWARE_TESTS=y`, `NETWORK_ENABLED=n`, `UI_ENABLED=n`, `DISPLAY_ENABLED=y`, `DISPLAY_TEST=y`, `I2S_SMOKE_TEST=y` |
| Eksperyment pulsowania OLED podczas radia | `HARDWARE_TESTS=y`, `NETWORK_ENABLED=y`, `UI_ENABLED=y`, `LCD_TIMING_PROBE=y` |

Skrócone symbole w tej tabeli mają prefiks `CONFIG_STREAMOO_`.
Sieć ma pierwszeństwo przed źródłem testowym. Przed powrotem do normalnego
radia wyłącz testy i diagnostykę, włącz sieć, ekran, enkoder oraz UI.
Szczegóły: [RELEASE_REVIEW.md](RELEASE_REVIEW.md).

Porównania FLAC można prowadzić przez wrapper, który sprawdza wybrany
backend przed wgrywaniem:

```sh
python3 tools/build_flac.py dr build
# Opcjonalne wgranie właśnie tego wariantu:
python3 tools/build_flac.py dr flash --port /dev/ttyUSB0
```

Dostępne nazwy: `dr`, `micro`, `esp`. Nie używaj samego `-B build/flac-dr`
jako sposobu wyboru dekodera. To osobne profile; zwykły `idf.py build`
używa głównego `sdkconfig`. Przed udostępnieniem logów usuń prywatne adresy,
nazwy sieci i stacji; nie dołączaj prywatnego pliku AP ani nagłówka z kluczem.

## Typowe problemy budowania

| Komunikat lub objaw | Działanie |
| --- | --- |
| `idf.py` nie znaleziono lub błędny Python | Aktywuj środowisko właściwej instalacji IDF w bieżącym terminalu |
| `StreamOO requires ESP-IDF 6.1` | Sprawdź `idf.py --version` oraz używany SDK; nie usuwaj kontroli CMake |
| Błąd profilu N16R8 lub GPIO | Popraw wskazane opcje w `menuconfig`; sprawdź, którego `SDKCONFIG` używa build |
| Zmiana defaults nie działa | Zmień istniejący `sdkconfig` przez `menuconfig`; defaults nie wymuszają jego nadpisania |
| Błąd prywatnej konfiguracji AP | Uruchom skrypt `configure_network.py --check`, sprawdź lokalizację i ewentualną ścieżkę zapamiętaną w CMake |
| Build przechodzi, ale brak Wi-Fi | Sprawdź prywatny plik AP oraz `NETWORK_ENABLED`; po pierwszym utworzeniu pliku wykonaj `reconfigure build` i wgraj nowy obraz |
| Brak pobierania zależności | Sprawdź dostęp do rejestru komponentów; zachowaj manifesty i lock, nie zastępuj wersji przypadkowymi bibliotekami |
| CMake ze Snap kończy się błędem środowiska | Na tym stanowisku pomaga `export PATH="/usr/bin:$PATH"` po aktywacji IDF, jeśli tam jest działający systemowy CMake/Ninja |
| Port zajęty lub brak uprawnień | Zamknij monitor/inny program, sprawdź kabel danych, nazwę portu i uprawnienia użytkownika do portów szeregowych |

Przy dystrybucji dołącz [LICENSE](../LICENSE), [zakres licencji](../LICENSING.md)
i właściwe [noty zależności](../THIRD_PARTY_NOTICES.md). MIT aplikacji nie
zastępuje warunków bibliotek i zasobów.
