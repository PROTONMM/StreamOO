# ADR0025 — osobne zadanie czytnika i generacje źródła

Data:2026-10-31. Status: wdrożone; odbiór sprzętowy06h OPEN.

## Kontekst

AAC06g odebrane logiem, odsłuchem i VOL/PLAY/STOP. Surowy byte_ring ma
jednego właściciela i nie może być bezpośrednio użyty przez zadanie sieciowe.
Oddzielamy testowy reader od właściciela dekodera przed integracją sieci.

## Decyzja

Nowy streamoo_buffered_source jest task-only adapterem wokół byte_ring.
Wszystkie operacje na danych/metadanych serializuje mutex FreeRTOS z IDF6.1.
Nie kopiujemy danych pod portMUX/wyłączonymi przerwaniami. Write/finish są
nieblokujące, read przy zajętym mutexie zwraca WAIT. Rewind, stop i snapshot
czekają timeout5ms przeliczony na ticki (minimum1 tick; przy100Hz10ms).
To limit oczekiwania API, nie gwarancja wall-time/scheduling/deadline.

Generacja startuje od1. Rewind pod mutexem inkrementuje generację i czyści
bufor, nie czeka na potwierdzenie starego readera. Reader pobiera token
PRZED przygotowaniem danych poza mutexem; write/finish odrzucają stary token,
więc opóźniony wynik nie zatruwa nowego cyklu. UINT32_MAX nie zawija się:
rewind odmawia operacji. Terminalny błąd jest lokalny dla generacji. Stop
blokuje kolejne publikacje. Clear surowego pierścienia występuje wyłącznie
pod mutexem; nie jest wywoływany równolegle z kopiowaniem.

Testowy fixture_reader: priorytet3, stos4096B, lokalny offset i chunk997B,
maksymalnie8 prób zapisu na przebudzenie. Snapshot generacji/stop, kopiowanie
flash do chunk poza mutexem, publikacja z tokenem; kursor rośnie tylko o
zaakceptowane bajty. Pełny bufor lub contention kończy burst, potem czeka na
notify lub10ms (minimum tick). Zmiana generacji zeruje jego lokalny offset.
EOF oznacza finish tylko dla własnego tokena. Rewind i stop budzą czytnik.

Audio_control nadal sam posiada dekoder, pump i producenta PCM. Nie zasila
już pierścienia. WAIT podczas startu/rewind/contention jest normalny; zachowuje
częściowy blok PCM, obsługuje komendy i czeka na kolejne uzupełnienie FIFO.
Stop/PLAY użytkownika nadal steruje gainem, nie zatrzymuje czytnika ani nie
pauzuje źródła. Piny, gain/tap, NVS, ISR i rebuffer bez zmian. Brak nowej
alokacji, flash, blokad źródła i dekodowania w callbacku I2S.

## Zamykanie i ograniczenia

Po zakończeniu używania dekodera release ustawia stop, budzi reader i czeka
do1000ms na semafor zakończenia. Reader po ack nie dotyka już współdzielonego
stanu i sam usuwa zadanie. Dopiero po ack wolno usunąć adapter/mutex/semafor
oraz zwolnić pożyczone PSRAM i stan dekodera. Jeśli stop/join zawiedzie,
logujemy błąd i zachowujemy żywe bufory, zamiast zwalniać pamięć używaną przez
zadanie. To awaryjne zachowanie może zatrzymać pamięć do restartu; brak
wymuszonego usuwania zablokowanego readera. Obecna ścieżka błędu aplikacji
najpierw zamyka I2S. Błędy alokacji przed startem readera sprzątają od razu.

Nie ma jeszcze HTTP/TLS, anulowania blokującego I/O, polityki reconnect ani
produkcyjnej zmiany stacji. Nowy reader pliku nie modeluje opóźnień sieci.
Generacja testowego EOF nie zastępuje generacji żądania playera. Bufor nadal
4KiB PSRAM do eksperymentu, nie produkcyjne256KiB. Obsługa timeoutów stop/join
na S3 i długotrwały test pozostają OPEN.

## API/zależności/licencja

Sprawdzono lokalne publiczne nagłówki FreeRTOS dostarczonego z przypiętym
ESP-IDF6.1: mutexy (priority inheritance), semafor binarny, bounded take,
xTaskCreate i task notifications. FreeRTOS-Kernel/LICENSE.md: MIT, bez
kopiowania jego kodu do własnych plików. Używamy już istniejącego SDK;
esp_audio_codec2.6.2, manifest/lock i notices bez zmian.

## Weryfikacja

168 testów hostowych PASS. Siedem nowych przypadków uruchamia rzeczywisty
adapter/ring z testowym shimem semaforów opartym na pthread: partial write,
EOF/stop, contention, błędy tworzenia, terminalny błąd, równoległy przesył
200003 bajtów przez31B pierścień i kontrolowane rewind pomiędzy pobraniem
tokenu a publikacją. Stare write/finish odrzucone, w nowym źródle tylko nowe
dane. ASan/UBSan tych7 przypadków PASS; bez LeakSanitizer/ThreadSanitizer.
Shim nie uruchamia FreeRTOS ani firmware reader_task i nie dowodzi czasów
scheduler/priority inheritance, lifecycle ack czy zachowania na dwóch rdzeniach S3.

Odbiór S3: independent reader, rosnące cycles/generation,193536 ramek/cykl,
stats_ok1, poprawny reader stack_hwm, stabilne PCM i heap, czysty odsłuch
kilku cykli i VOL/PLAY/STOP. WAIT może rosnąć na starcie/rewind bez underrun;
stale może być0 (nie wymuszamy hardware race). max_read nie jest czasem ISR.
EOF/reset oraz stabilność nowych dwóch zadań wymagają tego nowego odbioru.
