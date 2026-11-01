# ADR0099: zwykłe radio, opcjonalne testy i ocena MIT

Data: 2026-11-01. Status: przyjęte do implementacji; nowy odbiór S3 OPEN.

## Powód i decyzja

Użytkownik polecił przegląd całego kodu, wyłączenie testów z normalnego
firmware przez kompilację warunkową, ograniczenie UART i sprawdzenie MIT.
Zachowujemy testy hostowe, a sprzętowe źródła/klipy wybiera CMake/Kconfig.
DIAGNOSTICS i HARDWARE_TESTS domyślnie n. Zwykłe funkcje otrzymują nazwy
ENABLED; sdkconfig.rename zachowuje ustawienia dawnych czterech flag TEST.

Normalny logger WARN z wyjątkami INFO dla identyfikacji firmware i zmiany
stacji. Nie ma cyklicznego health/CPU/heap/PCM/FFT/OLED ani śladu gestów.
Underrun ograniczony do pierwszego i kolejnych co5s w sesji. Błędy
zachowane, szczegóły do builda diagnostycznego. Kontrola statystyk I2S
pozostaje osobno, co5s; crypto early init pozostaje funkcjonalne.
Nie zmieniamy rdzeni, stosów pozostałych zadań, buforów ani TLS verification.

Własny kod może otrzymać MIT przy ustalonych prawach, lecz całość zawiera
Modified MIT Espressif, Apache/BSD/OFL/MPL i bitmapy PL o nieznanych prawach.
Nie dodajemy pozornego blanket LICENSE. Nie wymieniamy zaakceptowanych
fontów bez oddzielnego kroku. Raporty: RELEASE_REVIEW i LICENSING_REVIEW.

## Walidacja

- Pełna suita:481 host tests PASS. Nowe regresje: zwykły player bez
  cyklicznego UART i z zachowaną ciągłością PCM; wstrzyknięty błąd I2S
  wykrywany bez raportowania; produkcyjny UI bez fikcyjnych danych/ownera,
  rysowanie bez IP; rzeczywisty wybór źródeł CMake; profiler U32 i U64.
- Native ESP-IDF6.1/ESP32-S3: zwykłe `idf.py reconfigure build` PASS;
  osobny świeży DIAGNOSTICS z U64 PASS; HARDWARE_TESTS z AAC/enkoderem
  i bez sieci PASS; skończony OLED + I2S bez UI/sieci PASS. Brak ostrzeżeń
  kompilatora w naszym kodzie; SDK nadal emituje uwagi Kconfig/CMake.
- kconfgen SDK: świeże sdkconfig.defaults daje zwykłe radio, WARN, bez
  testów/runtime stats; migracja czterech dawnych flag zachowuje y oraz n
  (niewidoczne zależne opcje pozostają wyłączone). Fragment PCM stress
  wybiera test/sinus/stall/tap100 i wyłącza sieć. Prywatny root/dr profil
  uaktualniony bez zmiany GPIO, sekretów i danych użytkownika.
- compile_commands normalnego main: wyłącznie main.c, network_portal.c,
  display_smoke.c i encoder_demo.c. W końcowym ELF nie ma markerów
  testowych stacji/TEST VOL/klipu ani CPU interval/task, OLED telemetry,
  IRQ edges, Decode perf/buffers/diag. Potwierdzono DIAGNOSTICS=n,
  HARDWARE_TESTS=n, runtime stats=n, log default2/max3/boot2, IRAM-safe I2S,
  backend dr, TCP23040/mailbox18. Kontrola24 hashy vendored FLAC PASS.
- Normalna binarka1531776→1520448B: mniej11328B. Statyczne sekcje ELF:

| Sekcja | Przed | Po | Różnica B |
|---|---:|---:|---:|
| .dram0.data |19208|19160|-48|
| .dram0.bss |30464|27264|-3200|
| Suma stałego DRAM |49672|46424|-3248|
| .iram0.text |90143|90015|-128|
| .flash.text |1047468|1039784|-7684|
| .flash.rodata |373468|369996|-3472|

Pomiar lokalnych ELF przed/po, nie nowy runtime heap. Dodatkowo normalny
start nie tworzy health taska ze stosem3072B. Rzeczywiste minimum/largest
po zmianie wymaga S3; nie dodajemy statycznego odzysku do dawnych minimów.
Diagnostyczna binarka1530864B; test AAC0xa8480B; skończony OLED/I2S0x3f940B.
Są to osobne profile, nie porównanie prędkości dekoderów.
`git diff --check` PASS. Testy hostowe nie mierzą sprzętowych instrukcji SIMD,
TLS z rzeczywistym serwerem ani UART/odsłuchu po tej zmianie.
Brak flash/erase przez agenta. Odbiór runtime/odsłuch tej wersji OPEN.
