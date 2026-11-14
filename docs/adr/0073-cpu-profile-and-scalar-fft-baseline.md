# ADR0073 — profil CPU i punkt odniesienia FFT bez SIMD

Data: 2026-10-31. Status: wdrożone do pomiaru sprzętowego.

## Cel

Krótkie luki wizualizacji HLS nie są potwierdzonym problemem wyjścia PCM:
poprzedni log ma `PCM=32`, `low=0`, `hard=0`, gdy jednocześnie rosną luki
droppable tap. Przed zmianą affinity, priorytetów, ringów lub struktur FFT
potrzebny jest pomiar obciążenia obu rdzeni i konkretnych tasków.

## Pomiar

Włączone są FreeRTOS runtime statistics z 64-bitowym `esp_timer` 1 MHz.
Co 10 s istniejący task `streamoo_health` wykonuje stały snapshot maksymalnie
48 tasków; nie ma alokacji, działania w ISR, callbacku I2S, dekoderze ani
workerze PCM. Pierwsza próbka tylko ustala bazę, kolejna daje:

```
CPU interval=10000ms core0_busy=..% core1_busy=..%
CPU task=<nazwa> load=..% affinity=0x.. stack_free=..B
```

`core*_busy` wynika z czasu `IDLE0`/`IDLE1`. `load` jest udziałem jednego
rdzenia w minionym interwale, więc np. `25.0%` oznacza około 2.5 s wykonania
tasku w oknie 10 s. Pokazywane jest sześć najbardziej aktywnych tasków,
z ich maską powinowactwa (`0x1` core0, `0x2` core1, `0x3` bez przypisania).
Snapshot jest diagnostyczny; przy ponad 48 taskach wypisuje ostrzeżenie zamiast
alokować pamięć lub prezentować niepełne dane.

`player`/dekoder jest nadal przypięty do core1, a `http_reader` do core0
zgodnie z ADR0069. `audio_levels` jest celowo nieprzypięty. Jego licznik
`core_samples=core0/core1` przy istniejącej linii `meters` mówi, na którym
rdzeniu wykonywały się iteracje meteringu; nie jest to czas CPU i nie należy
go tak interpretować.

## Inspekcja kompilatora

Zachowany artefakt bieżącego backendu `streamoo_spectrum.c` kompiluje się
Xtensa GCC 15.2.0 dla ESP32-S3 z `-Os`. Disassembly
`streamoo_spectrum_analyze` zawiera skalarne `mul.s`, `madd.s`, `msub.s`,
`add.s` i `sub.s`; nie zawiera instrukcji wektorowych/SIMD ani backendu
ESP-DSP. `CONFIG_STREAMOO_DSP_ENABLE_SIMD` jest obecnie tylko gotowością
konfiguracji, nie wyborem implementacji. Skalarne fused multiply-add FPU nie
jest SIMD.

Dlatego nie deklarujemy automatycznej wektoryzacji ani korzyści z wyrównania.
`streamoo_spectrum_t` zawiera float workspace w PSRAM o naturalnym
wyrównaniu 4 B. Najpierw porównamy obciążenie `audio_levels`, maksymalny czas
FFT i `core_samples` dla HLS/FLAC. Dopiero jeśli FFT jest istotnym źródłem
obciążenia, kolejnym krokiem będzie osobny, licencyjnie sprawdzony benchmark
ESP-DSP/sc16 lub własnego backendu z jawnym wyrównaniem danych — nie
niezweryfikowana zmiana layoutu.

## Odbiór

Po zwykłym flashu (bez erase) odczekać około 22 s na pierwszy pełny interwał.
Dla `TEST HLS 64k-heaacv2-stereo` oraz `TEST FLAC native lossless` przesłać
po dwa bloki `CPU interval` i następujące po nich linie `CPU task`, `player`
i `meters`/`FFT`. Dźwięk musi pozostać niezmieniony; szczególnie
`hard=0`. Na podstawie tych danych zdecydujemy, czy przypiąć metering do
rdzenia, ograniczyć jego pracę, czy rozpocząć benchmark struktury/FFT.

## Weryfikacja

383 testy hostowe oraz dwa wywołania `check_config.py --platform` PASS.
Kompilacja targetowym GCC15.2.0 sprawdziła syntaktycznie `audio_levels` oraz
`platform` z wymuszonymi nowymi opcjami FreeRTOS i nagłówkiem `esp_timer`;
nie jest to link ani build firmware. Pełny `idf.py build` pozostaje OPEN:
`export.sh` wskazuje brakujący lokalny virtualenv IDF6.1. Nie flashowano.
