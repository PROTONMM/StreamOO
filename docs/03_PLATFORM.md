# 03 — Platforma, architektura i zasoby

**USTALONE:** własne komponenty ESP-IDF zamiast monolitu Arduino. Rozdzielone warstwy: HAL display, UI, input, station storage, network/provision, web, player, stream reader, decoder, PCM output, DSP i app state.

## Wzorcowy przepływ
`HTTP(S) → compressed ring (PSRAM) → container/codec parser → decoder → PCM fanout → volume ramp → PCM FIFO → I²S DMA → PCM5102A`, a z fanout do osobnego `dsp_task`, następnie do widgetów.

## Własność stanów
- `player_manager`: wyłączny autor stanu odtwarzania. Web/enkoder wysyłają polecenia.
- `wifi_manager`: własność stanu połączeń i provisioning.
- `ui_manager`: aktualny ekran, animacje, wybór stacji.
- Operacje sieciowe nie blokują renderera ani wyjścia I²S; brak nieskończonego oczekiwania na mutex.

## Rdzenie
Startowe założenie: zadania Wi-Fi/stack zgodnie z konfiguracją ESP-IDF, ciężkie dekodowanie/DSP na rdzeniu dobranym pomiarami. Nie wymuszaj arbitralnie powinowactwa bez profilowania.

## Pamięć
- 16 MB flash z docelowymi partycjami OTA, NVS i LittleFS: **partycje do obliczenia na podstawie realnego rozmiaru firmware**.
- 8 MB PSRAM na compressed ring, większe PCM i dane WWW/stacji.
- DMA/I²S i najbardziej wrażliwe dane DSP we właściwej pamięci wewnętrznej, z wymaganym wyrównaniem.
- Nie alokuj dynamicznie w ścieżce audio czasu rzeczywistego. Wykrywaj utratę Wi-Fi, heap pressure, underrun i uszkodzone dekodery.

## Test zakończenia
Diagram zależności i właścicieli stanów, uruchomione podstawowe zadania, metryki heap/PSRAM, pomiar latencji i scenariusz braku Wi-Fi.
