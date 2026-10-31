# ADR0069 — rozdzielenie HTTP i dekodera FLAC między rdzenie

Data: 2026-10-31. Status: wdrożone do odbioru sprzętowego.

## Dowód

Log użytkownika dla `TEST FLAC native lossless` wskazuje, że pierwsze
przejście PCM do fazy wygaszania zbiega się ze wzrostem `src_wait` z 1 do
11. W tej samej próbce `low=1`, `hard=0`, `starts=2`; później `src_wait`
rośnie do 34 przy `low=2`, `starts=3`. `hard=0` oznacza, że nie doszło do
całkowitego opróżnienia PCM. Dźwięk jest celowo wyciszany przy 10 blokach
(50 ms) i wraca dopiero przy 24 blokach (120 ms), więc nawet krótka przerwa
dopływu może być słyszalna.

To nie jest dowód niewystarczającej mocy ESP32-S3: przy 240 MHz dekoder
zużywał około 1.14–1.34 s czasu ściennego w kolejnych pięciosekundowych
oknach, a najdłuższe wywołanie miało około 37 ms przy 24–31 blokach PCM.
Należy najpierw usunąć rywalizację zadań o harmonogram, zamiast osłabiać
ochronę PCM lub lokować używany przez ISR ring w PSRAM.

## Zmiana

ESP-IDF 6.1 w tej konfiguracji używa dwóch rdzeni, a task Wi-Fi jest
przypięty do rdzenia 0. `http_reader` jest teraz przypięty do rdzenia 0,
gdzie odbiera dane HTTP i zapisuje istniejący ring skompresowany w PSRAM.
`player` jest przypięty do rdzenia 1, gdzie dekoduje i produkuje istniejący
ring PCM w RAM wewnętrznym. Priorytety, rozmiary stosów, ringi, decoder,
I2S callback i zasady fade/rebuffer pozostają bez zmian.

Hostowe harnessy zachowują `xTaskCreate`; binarny build ESP sprawdza natywne
`xTaskCreatePinnedToCore`. Jednorazowe logi podają faktyczny rdzeń obu zadań.

## Odbiór

Zwykły flash bez erase, NVS i ponownego importu. Dla `TEST FLAC native
lossless` odtworzyć kilka minut oraz zmianę utworu. Oczekiwane: logi
`Task placement ... core=0/1`, mniej lub brak wzrostu `low` i `starts`;
`hard` pozostaje 0. Jeśli krótkie przerwy pozostaną, kolejnym krokiem będzie
zmierzenie czasu i stanu readera dokładnie przy przejściu `low`, a nie
zmiana progów PCM na ślepo. Odbiór sprzętowy pozostaje OPEN.
