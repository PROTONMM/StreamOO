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

`audio_output`: natywny I2S0 48kHz stereo32, callback IRAM wypełnia DMA,
bez blokującego write w ścieżce próbek. Aplikacyjne audio_tone/audio_probe
tworzą skończony test kanałów; nie są playerem, dekoderem ani DSP.

`audio_gain`: oryginalna tablica Q16 i rampa50ms dla PCM stereo32/48kHz,
bez float/heap/IO w próbkach. Aplikacyjny audio_control_probe ma osobnego
właściciela komend VolumeDelta/Toggle i testowy model; UI odczytuje snapshot.
Nie jest to jeszcze produkcyjny player ani analiza DSP.

`pcm`: własne pierścienie SPSC z acquire/release, stałe bloki stereo32/48kHz
i numery sekwencji. Polityka rezerwy/rebuffer oraz tap przed gain; bez RTOS,
heap i zależności UI. Aplikacja zapewnia RAM i pojedynczych właścicieli,
produkuje ton w zadaniu i odbiera tap w osobnym zadaniu.

`decoder`: cienki adapter Simple Decoder, MP3 i AAC-LC/ADTS z esp_audio_codec2.6.2,
przypięty manifestem/lockiem. `pcm_adapter`: własna konwersja16LE mono/stereo
48kHz do stereo32 oraz pętla PCM. Aplikacyjny codec_fixture dekoduje przed I2S
do PSRAM, a producent podaje przekonwertowane bloki do SRAM FIFO.
Nie jest to jeszcze streaming; AAC-Plus wyłączony, HE-AAC OPEN.
