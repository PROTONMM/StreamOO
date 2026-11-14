# ADR0064 — porcje wejścia dekodera i pomiar opóźnień FLAC

Data:2026-10-31. Status: kod/host/build PASS; odsłuch FLAC nadal OPEN/nieprawidłowy.

Użytkownik po ADR0062 nadal słyszy przerwy. Najnowszy fragment31..51s:
gen2 bez restartu,44100Hz, low5→11 i starts5→11, PCM schodzi do0, podczas
zrzutów compressed35838..138190B. Brak błędów dekodera/pamięci w tym fragmencie;
nie dowodzi to ich braku w całej sesji. max_fill90..91us/invalid0/hard0,
FFT available0. Odczyt compressed pochodzi co5s, a nie z chwili niedoboru;
nie rozstrzyga między opóźnieniem kodeka, producenta PCM i wejścia sieciowego.
Nie uznajemy wcześniejszej korekty za zakończoną naprawę odsłuchu.

## Zmiana

Produkcja używała257B wejścia na wywołanie parsera, wywodzących się z
pierwotnego testu fragmentacji. Wprowadzamy4096B w PSRAM, bez narzucania
minimalnego odczytu źródłu:1B, krótkie odczyty, częściowe consumption i WAIT
nadal obsługiwane. Zmniejsza to maksymalną liczbę wywołań API/locków dla
takiej samej ilości dostępnych danych, nie stanowi gwarancji poprawy na S3.
API przypiętego simple decoder2.6.2 już obsługuje dowolną długość wejścia.
Nie zmieniamy backendu FLAC, okna parsera ani rozmiarów PCM/ring/prebuffer.

Zachowujemy limit256 wywołań na packet i dodajemy limit około64KiB zużytego
wejścia (ostatnie wywołanie może przekroczyć go o<4096B). Dla pełnych porcji
to16 wywołań, zamiast256 drobnych; przy1B fragmentach dotychczasowy limit
pozostaje. To fairness/yield między pracami, nie limit czasu pojedynczego
binarnego dekodowania. Oddzielne source_waits oraz work_yields rozdzielają
rzeczywisty SOURCE_WAIT od dobrowolnego zakończenia porcji pracy.

## Pomiary zamiast kolejnej hipotezy o buforach

Player mierzy esp_timer_get_time wokół rzeczywistego SDK process oraz wokół
przygotowania240 próbek wyjściowych. To czas ścienny z ewentualnym wywłaszczeniem,
nie czysty CPU time. Raz na5s, oraz przy awarii:

`Decode perf: codec_max=...us block_max=...us codec_total=...us calls=... pcm_at_max=... pcm_max=... ring_at_max=.../... src_wait=... budget_yield=... reads=... fetched=... produced=... cache=... input=4096 refill=...`

- codec_max: najdłuższe pojedyncze wywołanie kodeka w sesji; ring_at_max to
  liczba gotowych bloków przed/po tym samym wywołaniu, pcm_at_max jego wyjście.
- block_max: najdłuższe przygotowanie bloku (kodek/parsing/konwersja/SRC).
- pcm_max: największy wyprodukowany pakiet; cache: bajty jeszcze nieprzetworzone.
- src_wait/budget_yield: liczniki od początku sesji; reads/fetched/produced
  pokazują postęp, nie utożsamiać fetched z pomiarem ruchu TCP po kablu.
- refill: czy aktywna jest bramka zapasu z ADR0062. input to pojemność,
  faktyczne odczyty mogą być krótsze.

PCM ma32 bloki po5ms, fade zaczyna się przy10; od pełnej kolejki do tego
progu jest110ms. codec_max trzeba oceniać wraz z ring_at_max, bo przy wejściu
z mniejszym zapasem dostępny czas jest krótszy. Snapshoty/maxima nie są śladem
wszystkich wydarzeń. Pozwolą jednak wybrać dalszy eksperyment: czas kodeka/
producenta, porcjowanie albo dopływ/refill. Nie wybieramy architektury na domysł.

Pomiary/counters/formatowanie tylko w tasku player/decoder; brak zmian
alloc/I/O/flash/FFT w callbacku I2S. Profil na stosie ownera, reset per nowy
decoder/sesja; wejście większe o3839B PSRAM. Bez zależności/formatu API,
GPIO/partycji/NVS/playlist i logo. Timer jest istniejącym API ESP-IDF6.1.

## Weryfikacja i następny pomiar PCB

372 testy hostowe PASS, live/actor ASanUBSan PASS (LSan off). Testy1B i
resamplera zachowane; model dwóch pakietów po96KiB dowodzi48 odczytów4KiB,
ciągłości, dwóch budget_yield i0 source_wait. Shim125000us w rzeczywistym
wrapperze potwierdza pomiar i powiązany rozmiar wyjścia, także reset sesji.
To nie pomiar kodeka Xtensa/I2S/RTOS na komputerze. Native build PASS
0x17aba0,90% app wolne; GPIO/diff PASS. Agent nie flashował/erase/eFuse.

Zwykły flash bez erase/FS/reimport. Ten sam TEST FLAC, po wystąpieniu przerw
zebrać2–3 kolejne zestawy phase=... oraz Decode perf... z tą samą generation.
Nie potrzeba ponawiać importu, strojenia FFT ani oceny logo. Dopiero te pomiary
pozwolą oddzielić długi decode od sieci/planowania. Wynik odsłuchu nowej
wersji i przyczyna wszystkich niedoborów pozostają OPEN.
