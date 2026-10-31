# ADR0024 — źródło skompresowane i ograniczony bufor

Data:2026-10-31. Status: AAC źródło/bufor potwierdzone logiem; odsłuch06g OPEN.

## Kontekst

AAC06f odebrane na S3, ale pump znał adres i rozmiar pliku flash. Przed
readerem sieciowym potrzebny jest interfejs niezależny od magazynu i poprawna
reakcja na chwilowy brak danych. Nie zmieniamy zaakceptowanej ścieżki I2S.

## Decyzja

Własny komponent compressed_source, bez zależności zewnętrznych/RTOS,
C11, dostępny dla ESP-IDF6.1 i testów hostowych. Nie kopiujemy kodu ani
nie zmieniamy wersji/licencji zależności. streamoo_byte_source_t udostępnia
read i rewind testowego źródła. Wyniki: DATA, WAIT, END, ERROR; DATA podaje
liczbę bajtów i znacznik last. WAIT nie oznacza EOF ani awarii.

Pierścień pożyczonej pamięci przyjmuje tylko dostępną część zapisu. Producent
przesuwa kursor wyłącznie o liczbę zaakceptowanych bajtów, przy pełnym buforze
wraca i próbuje później; brak nadpisywania/odrzucania skompresowanych danych.
Po finish nie przyjmuje zapisów. Błąd jest terminalny, ma pierwszeństwo nad
zaległymi danymi. Clear jest dozwolone dopiero po zatrzymaniu starego producenta.

Podczas otwartego źródła czytnik zatrzymuje ostatni bajt, aby po finish
przekazać EOS razem z rzeczywistym ostatnim wejściem. Nie polegamy na
niezweryfikowanym flush vendora z zerową długością. END bez wcześniejszego
finalnego DATA oznacza pusty plik i jest błędem obecnego testu audio.

Pierścień ma **jednego właściciela zadaniowego**. Wszystkie operacje muszą
być serializowane; nie jest współbieżną kolejką SPSC. Obecny feeder i decoder
należą do audio_control. Przyszły reader sieciowy wymaga osobnego adaptera
synchronizacji/komunikacji i generacji źródła; nie wolno podłączyć go wprost
z innego zadania do tej kolejki.

Pump nie zna już pliku ani jego adresu. Zachowuje fragment wejścia, pakiet PCM
oraz częściowo złożony blok wyjściowy (do240 ramek stereo32). READY publikuje
cały blok; WAIT pozostawia bufor wywołującego nietknięty. Ponowienie po WAIT
musi mieć tę samą liczbę ramek. FAILED zatrzaskuje błąd. EOF w teście sprawdza
liczbę ramek, resetuje dekoder i przewija źródło; to nie polityka produkcyjnego
STOP/reconnect ani obsługa zmiany stacji. Pełne PCM FIFO zatrzymuje pull;
WAIT kończy uzupełnianie bez publikowania niepełnego bloku, komendy pozostają
obsługiwane. Istniejący rebuffer PCM chroni I2S w razie dłuższego niedoboru.

## Eksperyment sprzętowy

Testowy feeder kopiuje oryginalny klip porcjami do997B do pierścienia4096B
w PSRAM, uzupełnia go przed próbą napełnienia FIFO i po rewind. Mały rozmiar
celowo wymusza fizyczne zawijanie i backpressure; **nie ustala** produkcyjnej
pojemności ani progów start/rebuffer sieci. Docelowy punkt wyjścia256KiB
pozostaje do pomiarów z rzeczywistym readerem. Decoder/work buffer i FIFO
pozostają w internal RAM. Brak pełnego PCM w PSRAM.

Log dodaje compressed/backpressure/waits. Backpressure liczy niepełne próby
zapisu, nie utratę danych. WAIT testujemy deterministycznie na hoście; normalny
feeder może go wcale nie wywołać. ISR, gain, tap, piny, NVS i start STOP bez
zmian. Sieć, HE-AAC, resampling i produkcyjny player nadal OPEN.

## Weryfikacja

161 testów hostowych PASS, w tym testy pierścienia, końcowego bajtu/EOF,
częściowego zapisu, zawijania, terminalnego błędu/clear i integracja pump z
małym pierścieniem/WAIT/przewijaniem. Ciągłość próbek sprawdzana przez granice
pakietów, chunków i cykli; brak modyfikacji output podczas WAIT. ASan/UBSan
15 przypadków pump i4 pierścienia PASS (bez LeakSanitizer). Backend testowy
nie uruchamia binarnego dekodera Espressif na hoście. Buildy w STATUS.

Odbiór S3: compressed_bytes4096, rosnące cycles/ok1, frames193536,
backpressure rośnie, queued/PCM stabilne, low0/hard0/invalid0 i czysty odsłuch
kilku cykli L/R/oba. Sprawdzić także VOL/PLAY/STOP. Dane heap/stos i czas read
nie są pomiarem jitter ani dowodem deadline w innym obciążeniu.
EOF podzielone na mniejsze wejścia i praca vendora z pierścieniem wymagają
nowego odbioru; wcześniejszy06f tego nie zastępuje.

## Wynik runtime AAC06g

Log c451643:10 cykli po193536 ramek, ok1, backpressure160→1662,
compressed3839B, waits0, max_read3829us, min stack_hwm22728B.
Do41s queued28/ACTIVE, low0/hard0/starts1, invalid0, tap_drop/gaps0,
max_fill133us. Heap internal209143B i PSRAM8337620B stabilne10–40s.
Gesty oraz zapis preferencji nie przerwały postępu audio. Odsłuch i skutki
VOL/PLAY/STOP nieopisane przez użytkownika, pozostają OPEN. waits0 nie
testuje niedoboru źródła na S3; test WAIT dotychczas tylko hostowy.
