# Komponenty ESP-IDF

Zaimplementowane: `board_config` (walidator), `platform` (metryki/start),
`hardware_probe` (opcjonalny surowy pomiar wejść), `ssd1322` (protokół i
transport SPI), `graphics` (canvas 4bpp, clipping i prymitywy), `text` (UTF-8, backend glifów i font ASCII/PL; licencja zmian PL do sprawdzenia). Aplikacja ma skończony test `display_smoke`. Nie zawierają
funkcji radia.

Kolejne samodzielne komponenty:
- `streamoo_input` — enkoder, debounce i rozpoznawanie gestów
- `streamoo_player` — centralny stan, komendy
- `streamoo_stream` — HTTP(S) input, bufory
- `streamoo_decoders` — adaptery kodeków, kontenery
- `streamoo_audio` — gain ramp, PCM queue, I2S
- `streamoo_dsp` — Hann, FFT, RMS, peak
- `streamoo_stations` — parser/import/eksport, repozytorium
- `streamoo_network` — Wi-Fi manager, provisioning, mDNS
- `streamoo_web` — nowy REST, frontend
- `streamoo_storage` — NVS/LittleFS, migracje danych

Nie dodawaj pustych `CMakeLists.txt` komponentów do builda: twórz je wraz z pierwszymi plikami implementacji i testów.

`text` zawiera także metryczny renderer glifów alpha 4bpp oraz zasób StreamOO UI16 na bazie Krypton (OFL); `graphics` komponuje alpha z tłem. Fonty nie zależą od sterownika ekranu.

`ui_widgets` renderuje trzy wizualizacje z niemodyfikowanego snapshotu i zależy tylko od `graphics`. Nie zawiera DSP; dane syntetyczne istnieją wyłącznie w aplikacyjnym `home_probe`.

`ui`: pojedynczy właściciel ekranu i wyboru na liście, animacja Q16, borrowed catalog snapshot i adapter komendy SelectStation. Zależy od graphics/text, nie od playera, GPIO i RTOS.

`ui` ma również pełny HOME ze spójnego snapshotu, walidację generacji i metadanych oraz cache źródeł. Pobieranie danych, synchronizacja czasu i audio nie należą do renderera.

Etap 05a: rzeczywisty komponent `input` to czysty dekoder gestów;
`hardware_probe` jest adapterem diagnostycznym GPIO z pollingiem co tick.
Docelowy transport szybkich zboczy oraz adapter sterowania pozostają otwarte.
