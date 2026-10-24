# Komponenty ESP-IDF

Zaimplementowane: `board_config` (walidator), `platform` (metryki/start),
`hardware_probe` (opcjonalny surowy pomiar wejść). Nie zawierają funkcji radia.

Kolejne samodzielne komponenty:
- `streamoo_display` — sterownik SSD1322, SPI transfer
- `streamoo_ui` — framebuffer 4bpp, renderer, fonty, widgety
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
