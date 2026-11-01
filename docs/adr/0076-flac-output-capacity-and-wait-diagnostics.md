# ADR0076 — pojemność wyjścia FLAC i przyczyny WAIT

Data: 2026-11-01. Status: częściowa poprawka i diagnostyka do odbioru S3.

## Dowód

Log użytkownika firmwareda70514/ELF26e6a4a2, FLAC gen3/44100Hz stereo24:
low rośnie1→17, starts2→17, hard0; src_wait18→188. CPU obu rdzeni poniżej
około49%, player zwykle36–41%. FFT ma średnią3.1ms, max do7.4ms, core0/priority4
i tylko kilka drop/gaps. Ten log nie wskazuje FFT lub całkowitej mocy CPU
jako przyczyny przerw audio. Fade/rebuffer jest rzeczywisty mimo hard0.

Przy73.307s następuje osobny błąd kończący sesję:
ESP_ERR_INVALID_SIZE/vendor-8, needed525312 > scratch524288,
used0/out0/pending4096. HTTP200, reader nadal otwarty. To potwierdzony
brak pojemności; nie jest dowodem, że bibliotekę blokuje sieć. Liczby nie
dowodzą też, jak parser wewnętrznie grupuje ramki. Odnotowano żądanie
pojemności, nie faktyczną produkcję525312B PCM na urządzeniu.

Dotychczasowy limit512KiB obejmował maksymalny pojedynczy blok stereo32
z65535 próbkami. Nie gwarantuje dowolnego wyjścia parsera/simple-decoder.
Przy innym utworze zmienia się rozmiar skompresowanych danych i wymagania
wyjściowe; ta obserwacja nie ustala jeszcze dokładnej przyczyny wszystkich low.

## Zmiana

Stały scratch w live pump zwiększamy do1MiB PSRAM, aby objąć wykazane
zapotrzebowanie525312B. Rezerwacja nadal przed I2S, bez realloc ani
alokacji wewnątrz decode/ISR. Wspólny pump zajmuje dodatkowe512KiB PSRAM
także dla innych kodeków sieciowych. Gdy zabraknie PSRAM, zachowany błąd
pcm_memory; internal ring32×240 rezerwowany jak wcześniej. Nadal istnieje
limit1MiB — nie deklarujemy wsparcia nieograniczonych paczek parsera.
Ta zmiana dotyczy konkretnego restartu, nie obiecuje naprawy wcześniejszych
krótkich rebufferów. Parser SDK/bootstrap128KiB pozostają; nie przywracamy
odrzuconego raw scanner ADR0061 ani nie zmieniamy progów PCM/prebuffer.

src_wait wcześniej sumował co najmniej dwie przyczyny: pusty ring (lub
pozostawiony końcowy bajt otwartego źródła) oraz nieudane try-lock. Dodano
nieblokujące liczniki empty_waits/busy_waits przy zachowaniu istniejących
semantyk synchronizacji. Jeden konsument aktualizuje atomic load/store,
bez sprzętowych RMW; getter nie czeka na mutex. HTTP publikuje snapshot
przy WAIT. To nie zmienia priorytetu ani przepływu danych.

Player przy pierwszym zaobserwowaniu zmiany low, przed kolejnym
uzupełnianiem PCM, loguje `Underrun gen/low/PCM/compressed/cache/pending/
wait_empty/wait_busy/refill`. To obserwacja zadania właściciela, nie pomiar
dokładnie w chwili ISR: jeśli dekoder zajmował CPU, log może być opóźniony.
Nie ma dodatkowego kodu/logowania w callbacku I2S. Liczniki są kumulacyjne,
na sesję; nie należy z pojedynczej wartości przypisywać przyczyny każdego low.

## Weryfikacja i odbiór

Regresja hostowa produkuje525312B symulowanego stereo24 PCM i sprawdza
wszystkie próbki do końca paczki oraz przejście na następną. To weryfikacja
pojemności/konwersji, nie wykonanie binarnego dekodera Espressif na hoście.
Test try-lock odtwarza WAIT przy8B w ring, następnie odczytuje dane do
pozostawionego ostatniego bajtu i weryfikuje osobny empty WAIT. HTTP testuje
publikację obu liczników; actor sprawdza log underflow i zachowanie sesji.
397 testów hostowych PASS, live10/HTTP23/actor11 z ASan/UBSan PASS (LSan
off), walidatory konfiguracji i diff --check PASS. Native ESP-IDF6.1 build
w build/simd-check PASS0x17e4a0,90% factory wolne. Nie zmieniono zależności.

Po zwykłym build/flash, bez erase/NVS/playlisty: TEST FLAC native lossless
minimum3–5min, także przejście na inny utwór. Zbierać Underrun, phase,
Decode perf/diag, Reader i CPU. Oczekiwane: brak restartu na needed525312;
jeśli low nadal rośnie, analizować przyrosty wait_empty/wait_busy, cache,
pending i compressed, nie zgadywać zmiany progów z samego src_wait.
Porównać FFT i bargraph; mały stabilny max_fft nie dowodzi ciągłości audio.
Płynność FLAC, rzeczywiste wyjście525312B i wpływ dodatkowej pamięci na S3
pozostają OPEN. Brak flash/erase/eFuse przez agenta.
