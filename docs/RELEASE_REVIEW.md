# Przegląd kodu i rozdzielenie testów — 2026-11-01

## Wynik przeglądu

Przejrzano własne moduły aplikacji, ich styki, CMake/Kconfig, ścieżki
logowania, hostowe testy regresji, manifesty i zachowane noty licencyjne.
Weryfikację zachowania stanowią testy hostowe i natywne kompilacje S3.
To przegląd inżynierski obecnego kodu, nie formalny dowód braku błędów,
nie audyt kodu binarnych bibliotek producenta ani nowy odbiór sprzętowy.

| Ustalenie | Wpływ | Zmiana |
|---|---|---|
| Opcje nazwane TEST sterowały zwykłym ekranem/siecią/enkoderem | Mechaniczne wyłączenie testów wyłączało radio | Odrębne ENABLED oraz migracja `main/sdkconfig.rename` |
| Testowe źródła main były kompilowane bez potrzeby | Pozostawienie testów w normalnej kompilacji, zależność od usuwania przez linker | Warunkowe listy SRCS i osadzanie klipów tylko przy odpowiednim wyborze |
| Okresowe logi playera zawierały kontrolę błędów I2S | Wyłączenie logów mogło usunąć wykrywanie awarii | Osobna kontrola co5s w zadaniu playera, niezależnie od DIAGNOSTICS |
| CPU/heap health task i liczniki IRQ pracowały dla normalnego radia | UART, snapshoty tasków i dodatkowe bufory/stos | Kompilacja tylko z DIAGNOSTICS |
| Adapter UI zawierał fikcyjne stacje, tytuły, słupki i lokalnego właściciela PLAY | Możliwe testowe zachowanie bez rzeczywistego właściciela | Syntetyczna gałąź tylko dla HARDWARE_TESTS/hosta; normalnie pusty katalog i jawne callbacki |
| Domyślne `ui.running` nie odpowiadało pustemu produkcyjnemu adapterowi | Niespójny stan przed pierwszym snapshotem playera | Produkcyjny adapter startuje STOP; regresja rzeczywistej gałęzi ESP_PLATFORM |
| Profiler zakładał U64 również przy ręcznym wyborze U32 w SDK | Niezgodny typ argumentu `uxTaskGetSystemState` | Użycie `configRUN_TIME_COUNTER_TYPE`, test kompilacji obu szerokości |
| Stare opisy README/not codec nie odpowiadały aktualnemu profilowi | Mylna instrukcja testów i ocena licencji | Bieżący punkt wejścia i osobny raport licencyjny; historia etapów pozostaje oznaczona |

## Zakres modułów i istotne granice

| Obszar | Co sprawdzono | Wynik / ograniczenie |
|---|---|---|
| Boot/board/platform/input | Walidacja N16R8/GPIO, kolejność crypto przed I2S, kolejki IRQ, przepełnienie, gates | Funkcjonalny driver enkodera pozostaje; wyłączono tylko ślady i zliczanie diagnostyczne |
| Audio output/gain/PCM | ISR, internal/IRAM, SPSC acquire/release, rampy, kolejność zamykania | ISR bez nowych alokacji, flash, FFT i blokującego I/O; parametry PCM bez zmian |
| Decoder/live decode | Granice PCM, source WAIT/EOF, generacje, backpressure, FLAC alternatywy | Nie zmieniano algorytmów ani rozmiarów buforów; zamkniętych dekoderów nie można tu przejrzeć źródłowo |
| HTTP/ICY/playlist/HLS/TLS | Limity URL/body, redirect, czas przed HTTPS, anulowanie/dispose, metadane | Zachowano weryfikację certyfikatów; obsługiwany podzbiór HLS bez zmian |
| Player | Kolejki komend, ten sam ID, STOP/fade, właściciel snapshotu, retry, błędy I2S | Zachowano ścieżki funkcjonalne także bez okresowych logów; nowe regresje release |
| Metering/spectrum | Osobne zadanie, droppable tap, cykl życia FFT, ograniczone sekcje krytyczne | Rzeczywisty DSP/SIMD pozostaje częścią radia; nie jest testem do wyłączenia |
| SSD1322/graphics/text/UI | Limity canvas/UTF8, clipping, dirty rows, źródło danych i stacji | Timingi panelu, logo i prezentacja po zmianie pikseli zachowane; usunięto syntetyczne treści z normalnej gałęzi |
| Stations/configuration/NVS | Walidacja CSV/JSON, refcount snapshotów, zapis tmp/flush/fsync/rename przed publish | Import nadal dopisuje; sekrety nie są eksportowane jako hasła; brak erase/reimportu |
| Network/web/clock | STA/AP, Origin/proof i limity requestów, dane zewnętrzne, ustawienia strefy | Otwarty panel LAN pozostaje zgodnie z decyzją użytkownika, nie dodano uwierzytelniania |
| Build/tools/licenses | Wersje, źródła warunkowe, profil Python/SDK, pochodzenie bitmap i zależności | Osobna ocena MIT; font PL nadal blokuje publiczne wydanie |

Historyczne nazwy plików `display_smoke`, `encoder_demo`, `input_probe`
obejmują dziś również zwykły adapter wyświetlacza/UI i driver wejścia.
Nie usunięto tych funkcji tylko ze względu na nazwę. Nie zmieniano hurtowo
nazw API ani publicznego REST. Testy komputerowe i obrazy wzorcowe pozostają
w `tests/` i nie wchodzą do listy źródeł normalnego firmware.

## Normalny build i UART

Zwykłe `idf.py build` / `idf.py -p /dev/ttyUSB0 flash` używa profilu radia:

- NETWORK_ENABLED, ENCODER_ENABLED, UI_ENABLED, DISPLAY_ENABLED: włączone.
- DIAGNOSTICS, HARDWARE_TESTS, DISPLAY_TEST, AUDIO_CONTROL_TEST,
  I2S_SMOKE_TEST i klipy MP3/AAC: wyłączone.
- Domyślny logger i bootloader: WARN. Maksymalnie kompilowane INFO.
  Po wejściu do app_main tylko tagi StreamOO i player mają INFO:
  identyfikacja firmware i nazwa/ID przy zmianie stacji.
- Brak cyklicznych raportów CPU/heap/PCM/FFT/OLED i każdej zmiany enkodera.
  Ostrzeżenia/błędy nadal są dostępne; underrun w danej sesji: pierwszy,
  później najwyżej co5s z aktualnym licznikiem. Szczegółowy dekoder/reader
  tylko w diagnostyce, TLS z faktycznym błędem pozostaje widoczny.

UART nie jest całkowicie niemy: ROM/SDK może wypisać dane jeszcze przed
app_main, a błędy wymagają zgłoszenia. Nie obiecuje się stałego spadku CPU
bez nowego pomiaru S3. Liczniki funkcjonalne, kontrola I2S oraz crypto
known-answer z ADR0096 pozostają: crypto przygotowuje współdzielone GDMA
przed I2S i nie może być wyłączone jako niepotrzebny test.

## Włączenie diagnostyki i testów

Najprościej: `idf.py menuconfig` → StreamOO Configuration →
`Compile detailed UART diagnostics and runtime profiling`.
Dla dłuższych pomiarów wybrać w FreeRTOS licznik U64 i zegar esp_timer.
Nie trzeba włączać źródeł testów sprzętowych do pomiaru prawdziwego radia.

Osobny katalog i nowy plik konfiguracji (polecenia w powłoce ESP-IDF):

```sh
idf.py -B build/diagnostics -D SDKCONFIG=sdkconfig.diagnostics.local.defaults -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.diagnostics.defaults' build
```

Nazwany tutaj SDKCONFIG jest prywatnym wygenerowanym plikiem, ignorowanym
przez wzorzec `*.local.defaults`; nie dopisywać go do repo. Overlay defaults
ma zastosowanie przy tworzeniu nowego pliku konfiguracji; istniejący plik
z ustawieniem DIAGNOSTICS=n wymaga menuconfig lub osobnej nowej konfiguracji.
Standardowy `build/` pozostaje zwykłym radiem. Nie uruchamiać fullclean
na współdzielonych katalogach zależności podczas równoległych buildów.

Testy sprzętowe: włączyć HARDWARE_TESTS (włącza też diagnostykę), następnie:

| Próba | Ustawienia |
|---|---|
| Sterowane enkoderem AAC/MP3/sinus | NETWORK_ENABLED=n, UI_ENABLED=y, AUDIO_CONTROL_TEST=y, wybrać źródło |
| Skończony OLED + L/R I2S | NETWORK_ENABLED=n, UI_ENABLED=n, DISPLAY_ENABLED=y, DISPLAY_TEST=y, I2S_SMOKE_TEST=y |
| Obciążenie/transfer OLED podczas radia | NETWORK_ENABLED=y, UI_ENABLED=y, LCD_TIMING_PROBE=y |

Samo HARDWARE_TESTS nie uruchamia automatycznie tonu. Sieć ma priorytet
nad fixture audio; do odsłuchu klipu trzeba ją wyłączyć. Przed powrotem
do radia wyłączyć HARDWARE_TESTS/DIAGNOSTICS, włączyć sieć i zwykły UI.
Stare nazwy czterech opcji są automatycznie migrowane przez ESP-IDF.

## Weryfikacja i otwarte punkty

481 testów hostowych i cztery profile native build PASS. Normalny obraz
ma1520448B, statyczny DRAM mniejszy o3248B względem wersji sprzed przeglądu.
Szczegóły i granice pomiarów: ADR0099.
Do odbioru na S3 po zwykłym flash: boot/logo/autoplay, enkoder i zmiana
stacji, STOP/wygaszenie/PLAY, aktualizacja WWW oraz FLAC/HLS przez kilka minut.
Potwierdzić ciszę okresowych logów i obecność nazwy przy zmianie stacji.
Ten przegląd nie zalicza nowego odsłuchu, pomiaru runtime heap ani testu
zaniku zasilania w trakcie zapisu flash. Presja WWW/przełączanie i długi
soak nadal są otwarte. Publiczna dystrybucja dodatkowo wymaga rozwiązania
kwestii opisanych w [LICENSING_REVIEW](LICENSING_REVIEW.md).
