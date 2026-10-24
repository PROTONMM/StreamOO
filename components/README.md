# Komponenty ESP-IDF

Zaimplementowane: `board_config` (walidator), `platform` (metryki/start),
`hardware_probe` (przechwytywanie IRQ, diagnostyka i kolejka gestów), `ssd1322` (protokół i
transport SPI), `graphics` (canvas 4bpp, clipping i prymitywy), `text` (UTF-8, backend glifów i font ASCII/PL; licencja zmian PL do sprawdzenia). Aplikacja ma skończony test `display_smoke`. Nie zawierają
funkcji radia.

Kolejne samodzielne komponenty:
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

`ui`: pojedynczy właściciel ekranu i wyboru na liście, animacja Q16, borrowed catalog snapshot i adapter komendy SelectStation. Zależy od graphics/text/ui_widgets oraz typów input, nie od playera, GPIO i RTOS.

`ui` ma również pełny HOME ze spójnego snapshotu, walidację generacji i metadanych oraz cache źródeł. Pobieranie danych, synchronizacja czasu i audio nie należą do renderera.

Etap 05a: rzeczywisty komponent `input` to czysty dekoder gestów;
`hardware_probe` od etapu 05b przechwytuje zbocza GPIO w IRQ i kolejce,
z licznikami strat. Podstawowe gesty potwierdzono na sprzęcie 5/6/7; szybkość graniczna pozostaje otwarta.

Etap 05c: ui_controls mapuje gesty A/B do UI i callbacku komend. Aplikacyjny
encoder_demo pokazuje testowy volume/play/station; zadanie ekranu jest
jedynym właścicielem UI. Brak prawdziwego playera; od 05d preferencje volume/widżetu zapisuje osobny serwis.

`preferences`: czysty format/debounce oraz backend NVS z ESP-IDF 6.1.
Jeden worker wykonuje zapis po 5s, UI wysyła kopie do skrzynki latest-wins.
Inicjalizacja/odczyt przy starcie, bez automatycznego erase. Bez zależności UI.
