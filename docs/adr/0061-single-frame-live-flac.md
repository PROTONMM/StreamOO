# ADR0061 — pojedyncze ramki live FLAC zamiast grupowania przez parser

Data:2026-10-31. Status: ODRZUCONE po odsłuchu S3, wycofane w ADR0062.

Log bca642b2 i użytkownik: wyraźna regresja, brak pamięci stage=memory oraz
ESP_ERR_INVALID_RESPONSE przy vendor=0, częste rebuffer. Hostowe CRC/oracle
nie wystarczały do sprawdzenia kontraktu binarnego raw dekodera. Poniżej
historyczne uzasadnienie eksperymentu; nie opisuje bieżącego firmware.

Użytkownik: TEST FLAC native lossless około1500k przerywa zależnie od
utworu, pojawia się Brak danych. Log b040430e: gen8/gen9 HTTP200,
44100/2/24, vendor-8, needed138240 > scratch131072. To kolejny rzeczywisty
limit wyjścia, mimo wcześniejszego zwiększenia okna w ADR0058. Wartość
138240 odpowiada5×4608×2×3. Częste low/starts również bez zmiany generation,
compressed czasem1, a innym razem>200KiB; invalid0/hard0. Z tego logu
nie można przypisać wszystkich przerw Wi-Fi ani FFT. Brak danych wizualizacji
może towarzyszyć niedoborowi audio i nie oznacza błędu rysowania.

## Decyzja i zakres

Nie podnosimy ponownie scratch ani deklarowanego max_frame, nie traktujemy
ich jako rozwiązania dowolnego grupowania. Samo needed nie dowodzi wnętrza
binarnego parsera: grupowanie/dostarczanie opóźnionych pakietów to hipoteza
zgodna z wielokrotnością bloku i obserwacjami. Wyłączamy tę warstwę dla
headerless live FLAC: własny bounded scanner przekazuje dokładnie jedną
ramkę do istniejącego `esp_audio_simple_dec` z `use_frame_dec=true`.
Przypięte API2.6.2 jawnie udostępnia ten tryb do pełnych ramek. Surowy
`esp_flac_dec_open` nie wymaga konfiguracji. Rejestracja FLAC bez zmian.
Nie importujemy implementacji kodeka ani GPL.

Pierwszy nagłówek nadal musi mieć jawne parametry i poprawne CRC8.
Granica następnej ramki wymaga poprawnego nagłówka/CRC8 ORAZ zerowego
CRC16 całej poprzedniej ramki z footerem. Dalsze nagłówki mogą używać
odwołań rate/depth do STREAMINFO. Nie zmieniamy bajtów ramek ani nie
pomijamy błędnego CRC; limit128KiB kończy błędną/za dużą ramkę jawnym
błędem. CRC16 polynomial0x8005, własna tabela generowana matematycznie.
Potrzebny lookahead to nagłówek następnej ramki; nie całe okno128KiB.
EOF sprawdza również CRC ostatniej ramki. Z jednego wejścia zawierającego
wiele ramek dekodujemy po jednej, zachowując niezużyty input i lookahead.

Zwykły plik z fLaC pozostaje na poprzedniej ścieżce parsera/metadanych.
Dla live nie generujemy już STREAMINFO do parsera. Reset czyści scanner,
ponownie rozpoznaje prefix, a przejście file/live rekonfiguruje handle
w tasku dekodera. Decyzja zastępuje bootstrap live z ADR0056/0058, nie
unieważnia ich testów samego czytnika nagłówka.

Cache128KiB+16B PSRAM alokowany podczas open FLAC, przed I2S. Scratch PCM
nadal128KiB, ring compressed256KiB, PCM32×240, gain/I2S i DSP bez zmian.
Brak alloc/flash/CRC/parse w callbacku I2S. Handle surowego dekodera
rekonfigurowany jednokrotnie po rozpoznaniu live, w tasku player. Nadal
limity mono/stereo16/24/32-bit8..48kHz oraz frame/PCM; nie obiecujemy
wszystkich legalnych rozmiarów bloków FLAC ani zmiany formatu live.

## Weryfikacja

370 testów hostowych PASS; scanner/header/adapter ASan/UBSan PASS,
LSan off. Realny scanner: szum24-bit z ramkami>20KiB i cisza, porcje
1/7/257/4096B, join od środka, CRC uszkodzony, limit, EOF. Granice bajt
w bajt zgodne z niezależnym FFprobe. Dodatkowo178 pełnych ramek z wcześniej
pobranej próbki tego samego RadioMast (3135562B) ma zgodne CRC8/CRC16 i
granice FFprobe; ostatni ucięty pakiet capture pominięty wyłącznie w oracle. Zmiana nagłówków na referencje
STREAMINFO z poprawnymi CRC daje identyczny PCM w niezależnym FFmpeg.
Shim SDK: live single-frame mode,1B/reset i pięć ramek w jednym wejściu
oddaje pięć osobnych wyjść; file path i błędy zachowane. Nie wykonujemy
binarnego dekodera Espressif na hoście, jego raw FLAC wymaga odsłuchu S3.

Native ESP-IDF6.1 build PASS0x17a9a0,90% app wolne, GPIO/diff PASS.
Zwykły flash bez erase/FS/reimport. Log oczekiwany:
`FLAC live: CRC-checked single frames, parser aggregation disabled`.
Ten sam stream minimum kilka minut/zmiana utworu, low/hard/starts,
compressed/PCM, heap/stack i brak vendor-8/needed138240. Jeśli przy braku
błędów nadal compressed schodzi do1 i low rośnie, oddzielnie zmierzyć
transport/przepływność i rebuffer; nie uznawać tego etapu za jego odbiór.
Agent nie wykonywał flash/erase/eFuse; partycje i prywatne playlisty bez zmian.
