# ADR0034 — pierwszy player HTTP/ICY

Data: 2026-10-31. Status: implementacja/build/testy hostowe PASS;
Pierwszy odsłuch kilku stacji i rate44,1/48kHz potwierdzone na ESP32-S3;
pozostały odbiór i pojedynczy retry OPEN (wyniki poniżej).
Użytkownik potwierdził listę z playlisty i wybór z etapu08a („Działa fajnie”).

## Granice kroku

Wybranie stacji z OLED wysyła ID/generację katalogu do ownera player.
Player rozpoczyna wybraną stację, obsługuje głośność, STOP/PLAY, utratę
Wi-Fi i retry tej samej pozycji. Nie wybiera automatycznie innej stacji.
Brak samoczynnego startu po reboocie; zapis ostatniego wyboru w NVS OPEN.
WWW pokazuje rzeczywisty status `source=network_stream`, running/buffering/volume;
sterowanie WWW/mobile/RC5 nadal osobno. Lokalny panel bez logowania pozostaje.

Pierwszy zakres: bezpośredni strumień MP3 albo AAC-LC/ADTS, HTTP lub HTTPS,
mono/stereo signed16, rate 8..48 kHz. Stałe wyjście 48 kHz stereo32. Nie
uznajemy HE-AAC/AAC+, FLAC, Vorbis/Opus, HLS, zagnieżdżonych M3U/PLS ani
uwierzytelnianych źródeł za obsłużone. HE-AAC pozostaje niezweryfikowane;
`aac_plus_enable=false` jak w odebranym dekoderze plikowym. Zmiana formatu
w trakcie strumienia kończy sesję i prowadzi do retry. URL kontenerów/list
może zostać odrzucony; importer nadal przechowuje takie adresy bez próby sieci.

## Moduły i ownership

- streaming: własny filtr ICY, live decode pump i niezależny HTTP reader.
- player: jeden task priority4, stack12288; kolejka16 komend od kontrolerów,
  stan publikowany jako spójny snapshot. Wyłącznie on zarządza dekoderem,
  PCM producentem, sesją i I2S. Odtwarzacz lokalnego fixture pozostaje osobnym
  testem; profil boot AP uruchamia tylko player sieciowy w stanie bez I/O.
- UI: wybór asynchroniczny, aktualizuje stację/PLAY/tytuł z ownera, bez
  optymistycznego podszywania się pod stan odtwarzania. IP nadal w trzeciej linii.

Komenda wyboru przejmuje referencję niezmiennego katalogu i sprawdza jego
generację. URL kopiowany do sesji przed rozpoczęciem I/O. Import z reorder
zachowuje aktywne ID i nie zamyka połączenia; usunięcie pozycji zatrzymuje,
bez losowego fallback. Sesja ma własny bufor i znaczniki metadanych; stary
reader nie może pisać do bufora następnej stacji.

Zmiana stacji/STOP/brak STA/AP-active: gain do zera, 100 ms na fade/DMA,
zamknięcie I2S, asynchroniczny stop readera. Zwalniamy sesję dopiero po ACK
czytelnika i quiescence I2S; brak force-delete lub zwalniania żywego I/O.
Podczas oczekiwania owner nadal odbiera komendy — najnowszy wybór wygrywa.
Zamykanie/DNS może potrwać dłużej od timeoutu transportu; UI nie czeka na nie.
Błąd zamknięcia I2S zachowuje zasoby, nie pozwala resetować żywego FIFO.

## Transport i bufory

HTTP reader priority3/stack10240, do256KiB skompresowanych danych w PSRAM,
przepływ przez istniejący serialized buffered_source. Brak całego klipu PCM.
Decoder startuje po8192 bajtach albo po końcu krótkiego źródła. To stały próg
PoC, nie docelowy adaptacyjny startup. PCM32×240 =160ms, dotychczasowy
reserve/fade/restart (10/24 bloków), bez zmiany algorytmu callbacka I2S.
Reader stosuje backpressure, nie nadpisuje bufora. Timeout read3s jest
ponawiany na tej samej sesji; dopiero pięć kolejnych timeoutów (~15s bez
postępu) lub twardy błąd/EOF kończy readera. Zbuforowane dane można odczytać
przed zgłoszeniem końca. Krótki underrun nie restartuje sam TCP.
Retry playera tej samej stacji 1/2/4/8/15s, reset backoff po PLAYING.
Ręczny nowy wybór i PLAY zerują poprzedni retry delay; nie czekają na
opóźnienie błędu wcześniejszego źródła.

SDK open/fetch_headers/read obsługuje chunk framing i ICY status line.
Własne Icy-MetaData:1, filtr icy-metaint z poprawnym stanem między fragmentami,
bloki0..4080B i ograniczony StreamTitle. Tytuł ma do256B; kontrolne znaki
zastępuje spacja, puste StreamTitle usuwa poprzedni tekst. UTF-8 rysuje
istniejący renderer; osobna konwersja historycznych kodowań ICY OPEN.

Metadane mają pozycję w skompresowanym audio, do64 markerów w PSRAM. Stan
widoczny dla dekodera przesuwa się dopiero z konsumpcją danych, a nie po
samym odczycie z sieci. Granica publikacji jest przy kolejnym odczycie pumpa;
możliwe wyprzedzenie o PCM/decode ahead (~160ms), nie cały256KiB bufor.
Przy skrajnie małym metaint i pełnej kolejce pomijane są pośrednie tytuły,
nie audio. Nowa generacja/STOP/błąd/brak sieci czyści tytuł.

## PCM, I2S i pamięć

Oryginalny task-only liniowy resampler ze stałą fazą wymierną, uwzględniający
fragmentację i WAIT, konwertuje 44,1kHz i inne8..48kHz do48kHz, duplikuje mono.
To PoC, nie wysokiej jakości band-limited SRC/DSP. Bez float/FFT/alokacji
w IRQ. Właściwy vendor decoder MP3/AAC-LC nie jest uruchamiany na hoście;
nowa integracja 44,1kHz potwierdzona odsłuchem i logiem poniżej.

PCM storage, render i scratch dekodera wewnętrzne; skompresowane dane,
metadane i HTTP URL w PSRAM. Stary testowy pcm_storage32 bloków przeniesiono
z bezwarunkowego BSS do alokacji internal przy starcie testu — profil sieciowy
nie rezerwuje drugiego nieużywanego FIFO (około60KiB). Alokacja tylko task/startup,
nie w callbacku. Mbed TLS external allocation w PSRAM, pełna wiązka CA.
IRAM fill/pcm_fill i DRAM pipeline/ring potwierdzone w ELF; to nie pomiar
opóźnień ani dowód odsłuchu.

## HTTPS i zależności

Istniejący przypięty ESP-IDF6.1; bez nowych paczek rejestru. Przejrzano API,
licencje, SDK parser ICY/unknown-length i timeout EAGAIN. esp_http_client i
esp_netif/SNTP Apache-2.0, HTTP Parser2.7.0 MIT, Mbed TLS4.1.0 wybrane Apache-2.0.
Mozilla CA data MPL-2.0: licencja i odpowiadający PEM źródłowy dołączone,
`licenses/ESP-HTTP-TLS-NOTICE.md`. Bez kopiowania yoRadio/GPL implementacji.

Cert bundle i hostname verification włączone, bez insecure fallback i bez
przejścia HTTPS→HTTP przy redirect. Do5 przekierowań, bez credentials w URL
lub Location; Content-Type rozpoznane tokenowo, octet-stream ma bounded probe.
Po pierwszym STA startuje SNTP pool.ntp.org. HTTPS może ponowić próbę, zanim
zegar się zsynchronizuje. Brak synchronizacji/niezaufany certyfikat nie wyłącza
weryfikacji. Odświeżanie CA/OTA/mDNS i polityka czasu docelowa nadal OPEN.

## Weryfikacja i odbiór

247 testów hostowych PASS: ICY w fragmentach, maksymalny blok/metadane przy
pozycji danych, sniff po1B, redirect/TLS downgrade i credentials, błędne MIME,
read timeouty/cancel/ACK, cleanup błędu taska, live EOF bez loop, stereo48k,
resample44,1k i mono22,05k z WAIT, zmiana formatu. Actor test: gate AP,
queue/generation, szybka zmiana podczas ACK, retry/backoff i pominięcie jego
opóźnienia po ręcznym wyborze/PLAY, reorder i usunięcie
aktywnej pozycji. UI czeka na snapshot ownera, portal pokazuje źródło sieciowe.
Transport/RTOS/decoder/I2S w tych testach są deterministycznymi atrapami;
nie są pomiarem prawdziwego TCP/TLS, kodeka, DMA ani współbieżności SMP.
Targeted actor/HTTP/PCM ASan/UBSan PASS, LSan off, bez TSan.
Native build ESP-IDF6.1 PASS; odbiór według NETWORK_BRINGUP.
Agent nie wykonywał flash/erase/eFuse ani połączeń do prywatnych stacji z hosta.

OPEN: odsłuch MP3/AAC-LC, 44,1kHz/48kHz, HTTP/HTTPS/ICY, heap/stosy/jitter,
szybkie przełączanie i STOP bez trzasków, retry po zaniku sieci, import
podczas grania. Trwałe ID przy edycji i ostatni wybór NVS, wysokiej jakości
SRC, reszta kodeków, DSP/tap/widgets z rzeczywistego audio i API sterowania
pozostają dalszymi etapami. Nie oznaczać projektu jako ukończonego.

## Pierwszy wynik sprzętowy

2026-10-31: użytkownik sprawdził kilka stacji i potwierdził dobry dźwięk.
PLAYING44100/48000, PCM32, invalid/low/hard0, starts1 na widoczną sesję;
max_fill111–112us. HWM player min8704 B, reader6404 B; internal heap w stanie
odtwarzania61139–61599 B, min39844/largest31744 B, PSRAM min8031908 B.
Szczegóły w STATUS. To odbiór podstawowego toru, nie wszystkich protokołów/kodeków.

Gen7 raz zgłosił HTTP200/ESP_FAIL przy decoder0/vendor0 i format22050/2/16;
retry gen8 odtworzył44100. Powód guardu nie jest rozróżniany w obecnym logu.
Przed naprawą dodać fail_reason; nie przypisywać przyczyny HE-AAC/formatowi
bez dowodu. Rozszerzone testy z sekcji OPEN pozostają, podstawowy odsłuch
i konwersja44,1→48kHz zostały już zaliczone.
