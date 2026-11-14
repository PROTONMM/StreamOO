# ADR0038 — osobny odbiorca tap i rzeczywiste wskaźniki stereo

Data: 2026-10-31. Status: przyjęte; odbiór sprzętowy07b OPEN.

## Decyzja i własność

Własny komponent metering opakowuje levels w osobne zadanie priority1,
stack4096B. Kontekst około10KiB z czterema blokami tap i buforem konsumenta
alokowany w internal RAM przed otwarciem I2S. Używa istniejącego SPSC
streamoo_pcm_ring i istniejącego przed-gain tap w pcm_fill. Callback nie
wykonuje analizy, alokacji, flash, I/O ani FFT. Ring drop-newest; audio nigdy
nie czeka na pomiary. Konsument odczytuje maksymalnie4 bloki, potem delay5ms
(minimum1 tick). Rzeczywisty koszt kopii i obciążenie wymagają pomiaru S3.

Player jest jedynym właścicielem cyklu życia. Otwiera meter przed I2S,
przekazuje jego tap do pipeline, a po zamknięciu I2S prosi konsumenta o STOP.
Dispose zwraca false do ACK; dopiero potem zwalnia kontekst. Konsument po
release ACK nie dotyka kontekstu, usuwa własne zadanie; pamięć stosu/TCB
odzyskuje FreeRTOS. Brak resetowania pierścienia przy aktywnym producencie.
Błąd alokacji lub utworzenia zadania wyłącza pomiary dla tej sesji, bez
zatrzymywania odtwarzania. Brak dodatkowego zadania w AP/przed wyborem stacji.

Snapshot zawiera RMS/peak, generation, czas ukończenia okna, liczniki okien
i luk, max czas pracy jednej iteracji oraz stack_hwm. Krótkie sekcje
krytyczne chronią kopiowanie snapshotu, obliczenia poza nimi. Timestamp
odświeżany dopiero po kompletnym oknie. Wynik starszy niż200ms unieważniany.
Player publikuje poziomy tylko dla PLAYING i bieżącej generation; STOP,
zmiana stacji, retry, buffering/brak Wi-Fi/AP nie mogą pokazać starego wyniku.
Getter playera również sprawdza wiek, gdy owner zostanie opóźniony.

Adapter UI kopiuje poziomy do bargraph i analog. Skala to liniowa amplituda
Q15, okno40ms; smoothing, peak-hold oraz kalibracja dB/VU nadal OPEN.
Spectrum nadal „Brak danych”, bez syntetycznych pasm; FFT osobno.
Wiersz IP/VOL i dotychczasowe sterowanie bez zmian. Wciśnięcie+obrót wybiera
widżet. Źródło referencyjne z syntetycznymi widgetami nadal oznaczone TEST.

## API, zależności i sprawdzenie

Brak nowych bibliotek zewnętrznych i importów kodu. Sprawdzone lokalne API
przypiętego ESP-IDF6.1: heap_caps_calloc INTERNAL/8BIT, esp_timer_get_time,
xTaskCreate, vTaskDelay, vTaskDelete, uxTaskGetStackHighWaterMark i portMUX.
FreeRTOS z SDK pod dotychczasową licencją MIT, adaptery SDK Apache2; brak
zmiany wersji. Atomiki stop/done używają wyłącznie load/store, bez RMW.

284 testy hostowe PASS. Rzeczywisty worker z deterministycznym schedulerem
pthread:6 scenariuszy obejmuje błędy alokacji/zadania, pełne okno, generację,
ACK przed free, przeterminowanie/cofnięcie czasu, przepełnienie tap/luki
i nową sesję bez poprzedniego wyniku. Actor z atrapą metering:5 scenariuszy
chroni odtwarzanie przy braku pomiarów oraz przed starą generacją; istniejące
STOP/retry/szybkie wybory nadal przechodzą. UI testuje oba mierniki, brak
FFT i czyszczenie. Metering/actor/controls ASan/UBSan PASS, LSan off; bez TSan.

Native build ESP-IDF6.1 PASS, obraz0x15bcd0 (7% wolne). Pcm_fill pozostaje
w IRAM. Host nie potwierdza RF/DMA/SMP/deadline ani fizycznego OLED.
Procedura DSP_BRINGUP; bez flash/erase/eFuse wykonanych przez agenta.
