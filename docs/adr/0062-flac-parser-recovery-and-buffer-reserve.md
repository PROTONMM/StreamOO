# ADR0062 — wycofanie raw FLAC, pełny PCM i zapas danych sieciowych

Data:2026-10-31. Status: host/build PASS, odsłuch S3 nadal nieprawidłowy.

Kolejny log gen2 potwierdza niedobory PCM bez widocznego błędu kodeka/pamięci
w tym fragmencie. Kontynuacja pomiarowa w ADR0064; nie uznawać tej zmiany
za zakończenie naprawy FLAC.

Użytkownik potwierdził pogorszenie po ADR0061. Log bca642b2: wielokrotne
stage=memory/ESP_ERR_NO_MEM przy starcie, później raw single-frame stream
44100/2/24 z compressed=1, low/starts rosnącymi około2/s i ostatecznie
ESP_ERR_INVALID_RESPONSE/vendor0. Testy CRC/oracle nie dowiodły kontraktu
surowego binarnego dekodera na S3. Wycofujemy eksperyment z4e20e67; nie
uznajemy go za naprawę. Zachowujemy ADR0061 jako odrzuconą decyzję.

Wracamy do sprawdzanego parsera SDK2.6.2 i dotychczasowego bootstrap
STREAMINFO dla headerless FLAC (ADR0056/58). Usunięty dodatkowy scanner,
CRC cache128KiB i tryb use_frame_dec. Brak nowych zależności/kodu zewnętrznego.
Nie twierdzimy już, że potrzebne138240B na pewno oznacza grupowanie pięciu
ramek: było to niepotwierdzone przypuszczenie. Oryginalny log jednoznacznie
pokazywał jedynie niedostateczną pojemność wyjścia131072B.

Scratch PCM512KiB PSRAM: obejmuje65535 próbek/blok ×2 kanały ×4B =524280B
(górna granica pojedynczego obsługiwanego bloku), w tym zgłoszone138240B.
To stała alokacja przed I2S, bez realloc w callbacku/decode. Nie gwarantuje
nieograniczonych pakietów ani wszystkich FLAC: encoded maxframe nadal128KiB,
rate8..48k, mono/stereo16/24/32-bit. Zmiana nie przypisuje konkretnego
blocksize temu strumieniowi bez dalszych pomiarów. Dodatkowo384KiB scratch,
zamiast dodatkowego128KiB cache z ADR0061; duża pamięć wyłącznie w PSRAM.

PCM ring32×240 i pojedynczy blok internal rezerwujemy PRZED codec_open,
aby alokacje kodeka nie fragmentowały miejsca na ciągły ring. Log braku
pamięci teraz stage=pcm_memory oraz osobne ring/block/scratch booleans;
codec_open nadal odrębny etap. Sam log poprzedni nie dowodzi, który malloc
nie powiódł się; test kontroluje kolejność, odbiór realnego heap nadal OPEN.

FLAC: start po około500ms danych według icy-br, domyślnie64KiB gdy brak
bitrate, ograniczenie32..192KiB poniżej istniejącego ring256KiB. Dla1500k
93750B zamiast8192B. Po WAIT przy skompresowanym wejściu<=1B i niskim PCM
czekamy na odbudowanie tego zapasu, bez zamknięcia TCP/dekodera i bez
resetowania generation. EOF nie blokuje tej bramki. Zapobiega wznowieniom
na zbyt małym dopływie, nie gwarantuje stabilności przy sieci stale wolniejszej
od źródła. MP3/AAC i pozostałe kodeki zachowują próg8192B. Bez zmian ISR,
I2S/gain/FFT/pinów/partycji/NVS/prywatnych playlist.

Host370 testów PASS, live/actor ASan/UBSan PASS (LSan off). Regresje:
PCM138240B i maksymalny blok stereo32-bit524280B, wcześniejsze formaty,
rezerwacja ring przed otwarciem kodeka; symulowany FLAC preroll/refill
zachowuje jedną sesję i czeka do pełnego progu, granice uint32/pojemności.
SDK/RTOS to atrapy, nie odsłuch/pomiar przepływności i heap na S3.
Native ESP-IDF6.1 build PASS razem z logo ADR0063, obraz0x17a8a0,90% app.

Wgrać zwyczajnie bez erase/FS/reimport, sprawdzić FLAC kilka minut i zmianę
utworu. Oczekiwany log `FLAC live frame: supplied validated STREAMINFO`;
nie powinno być `CRC-checked single frames`. Zbierać low/hard/starts,
compressed/PCM i heap/min/largest; pojedynczy dłuższy rebuffer ma teraz
celowo odbudować zapas. Jeśli nadal brak ciągłości, potrzebne pomiary czasu
dekodowania i dopływu HTTP; nie zgadywać przyczyny z samego bitrate1500k.
Agent nie wykonał flash/erase/eFuse. Wynik sprzętowy pozostaje OPEN.
