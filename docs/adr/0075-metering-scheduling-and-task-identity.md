# ADR0075 — terminowe RMS/FFT i tożsamość tasków w profilerze

Data: 2026-11-01. Status: wdrożone do odbioru S3.

## Dowód z urządzenia

Log firmware6f27c8b/ELF928969ce potwierdza wykonanie backendu SIMD.
Zwykłe feed z FFT zajmuje około2–4ms. W HLS64k HE-AACv2, gen4, maksimum
czasu ściennego rośnie jednak do513616us, a max_work do514331us. W tym samym
odcinku PCM pozostaje32, low0/hard0/starts1, src_wait0; nie ma dowodu na
niedobór sieci lub PCM. Nie interpretujemy czasu ściennego jako kosztu CPU
samego FFT. Luki tap wymuszają poprawny reset kolektora i opóźniają następną
pełną klatkę; sam limit odświeżania lub dłuższe trzymanie obrazu tego nie naprawia.

HLS około121–131s: core0 busy35.7–36.3%, core1 busy59.3–59.7%;
player57.0–57.6%, reader14.4–14.7%, audio_levels7.5% jednego rdzenia.
To nie jest wyczerpanie całkowitej mocy CPU. Sam log nie wskazuje dokładnie,
które wywłaszczenie składa się na każdy skok513ms. Wskazuje natomiast
podatność na głodzenie zadania priority1 przez reader3/dekoder4.

Pierwsza stacja HE-AAC, gen2: audio_levels po użyciu FPU ma affinity0x2
i core_samples12/2544, podczas gdy dla HLS i FLAC przypina się do core0.
Lokalny ESP-IDF6.1 `portasm.S`, `_frxt_coproc_exc_hook`, przypina task
do bieżącego rdzenia przy użyciu koprocesora. Stary xTaskCreate bez affinity
nie gwarantował więc migracji podczas analizy, wbrew pierwotnej intencji.

FLAC gen6 w dostarczonym odcinku: low0/hard0/starts1, średni FFT około3.6ms,
max23.13ms, mniejsze ale nadal obecne drop/gaps. To odbiór tego fragmentu,
nie pełny soak ani potwierdzenie poprawki z tego ADR.

## Zmiana

Tworzymy audio_levels jawnie na core0, priority4, stack4096B. Dekoder/player
pozostaje core1 priority4, reader core0 priority3. RMS/FFT może więc
wywłaszczyć reader przy lokalnej pracy TLS/HLS; zadania Wi-Fi/TCP/IP nadal
mają wyższe priorytety. Worker nadal przetwarza maksymalnie4 bloki przed
vTaskDelay i oblicza FFT najwyżej10Hz. Ring pozostaje4 bloki/20ms, zawsze
droppable. Nie zmieniamy I2S, progu rebuffer, algorytmu SIMD ani wygaszania UI.
Log startu tasku i core_samples potwierdzają nową politykę na urządzeniu.

Dodatkowo log po zmianie stacji zawierał niemożliwe
`http_reader load=204608176.2%`. Profiler porównywał wyłącznie adres TCB,
który allocator może wykorzystać dla nowego tasku. Porównujemy teraz parę
handle/xTaskNumber i wymagamy niemalejącego runtime. Nowy task rozpoczyna
własną bazę; jego pierwszy niepełny interwał nie jest raportowany jako load.
To naprawa metryki, a nie zmiana rzeczywistego obciążenia.

## Weryfikacja i odbiór

396 testów hostowych PASS. Harness rzeczywistego meteringu sprawdza żądanie
core0/priority4, lifecycle/ACK, awarie alokacji/tasku, drop i reset okien.
Nowy harness rzeczywistego profilera odtwarza zwykłe delty, powtórzony adres
z mniejszym i większym runtime nowego tasku oraz reset licznika.
Metering9 i profiler4 testy z ASan/UBSan PASS (LSan off). Walidatory GPIO/
platformy oraz diff --check PASS. Native ESP-IDF6.1 build w build/simd-check
PASS0x17e2c0,90% factory wolne. Brak nowych zależności/zmian konfiguracji.

Na S3: HLS64 HE-AACv2 minimum60s FFT i30s bargraph, potem ponownie FFT.
Tak samo bezpośredni HE-AAC i FLAC, kilkukrotna zmiana stacji. Oczekiwane
`metering: Task placement: RMS/FFT core=0 priority=4`, affinity0x1,
core_samples=N/0, mniej skoków czasu i drop/gaps; low/hard/starts audio
nie powinny się pogorszyć. Bez ponownego logu poprawa płynności pozostaje
OPEN. Przy dalszych lukach potrzebny pomiar czasu gotowości/wywłaszczeń,
zamiast utożsamiania max_fft z czasem obliczeń. Agent nie flashował.
