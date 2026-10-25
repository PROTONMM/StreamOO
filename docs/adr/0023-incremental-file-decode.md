# ADR0023 — dekodowanie pliku podczas odtwarzania

Data:2026-10-25. Status: AAC06f odebrane logiem i odsłuchem; MP3 incremental OPEN.

## Kontekst i decyzja

MP3 i AAC-LC z pliku przeszły PoC, AAC także czysty odsłuch. Usuwamy
pełny1MiB bufor rozkodowanego klipu w PSRAM. To kolejny mały krok przed
źródłem sieciowym; nadal oryginalny, zapętlany plik we flash.

Nowy task-only streamoo_decode_stream przechowuje257B wejścia i8192B PCM
(jeden pakiet), zachowuje niezużyte wejście i niewydany ogon PCM. Na żądanie
producenta konwertuje dokładnie240 ramek, także przez granicę pakietu/EOF.
Producent wywołuje read tylko przy miejscu w FIFO, więc pełne FIFO zatrzymuje
dekodowanie. Przy starcie przygotowuje160ms zapasu, później dekoduje podczas
pracy I2S. Nie ma nowej alokacji całego PCM ani odczytu PSRAM w ISR.

Decoder i stan streamu należą do istniejącego zadania audio_control, priorytet4,
stos24KiB. Zadanie nadal obsługuje komendy i okresowo uzupełnia FIFO; czas
read/reset może opóźnić obsługę komendy. Nie deklarujemy deadline ani jitter.
Callback I2S/tap/gain/ochrona niedoboru pozostają bez zmian i bez dekodera,
alokacji, flash, FFT i blokującego I/O. Stan roboczy streamu w internal RAM.
Sam vendor może zarządzać własną pamięcią w zadaniu; nie deklarujemy braku
alokacji wewnątrz dekodera/resetu.

W EOF sprawdzamy liczbę ramek192000..194560 AAC lub194688 MP3 i resetujemy
parser/dekoder, bez ponownego open. API esp_audio_simple_dec_reset sprawdzone
w nagłówku przypiętego esp_audio_codec2.6.2: resetuje parser i dekoder. To
polityka zapętlania testu, nie produkcyjna semantyka STOP/reconnect. STOP
nadal wycisza, źródło biegnie. Nie dodajemy sieci, resamplingu ani HE-AAC.
Licencje/zależności bez zmian; żaden kod zewnętrzny nie został skopiowany.

Błędy formatu, zakresów/progresu/EOF/resetu zatrzaskują błąd streamu;
nie publikujemy niepełnego bloku FIFO ani zastępczego tonu. Istniejąca ścieżka
błędu zatrzymuje audio. Backend ma jednego właściciela, nie jest współbieżny.

## Weryfikacja

Testy hostowe z kontrolowanym backendem: ogony1024→240, wyjście bez zużycia
wejścia, częściowe zużycie, wiele chunków257B, ciągłość przez EOF, pełny
zakres signed16, brak samoczynnego decode bez pull, błędy backendu/resetu,
no-progress, bounds, format i niepełna ramka PCM. Nie uruchamiają binarnego
dekodera Espressif na hoście. Wyniki build/test w STATUS.

Odbiór S3: start streaming source ESP_OK, rosnące cycles, ok=1,
frames193536 zgodne z referencją, PCM low0/hard0/invalid0, stabilne heap/stos,
czysty odsłuch kilku cykli oraz VOL/PLAY/STOP. max_read_us mierzy pojedynczy
pull240 ramek (włącznie z resetem), nie cały burst producenta ani ISR.
EOF/reset i koszt dekodowania w trakcie DMA wymagają tego nowego odbioru.

## Wynik odbioru AAC

Log3b18324: streaming source ESP_OK, work_bytes8528, cycles1..5/ok1,
frames193536, max_read4034us, stack_hwm22728B. PCM queued27..30/ACTIVE,
low0/hard0/starts1, invalid0, tap_drop/gaps0, max_fill130us.
Heap internal211223B i PSRAM8341720B stabilne w próbkach10–20s.
Użytkownik potwierdził „Dżwięk OK”. Decode/reset podczas DMA i odsłuch AAC
zaliczone. MP3 incremental, nowe próby VOL/STOP oraz długotrwałe obciążenie
pozostają osobnymi testami; wyniki AAC ich nie zastępują.
