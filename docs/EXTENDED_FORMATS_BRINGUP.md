# Odbiór HE-AAC, FLAC, Ogg Vorbis/Opus i M3U/PLS

Wgraj zwykłym `idf.py -p PORT flash` cały bieżący build, łącznie z tablicą
partycji; bez erase. Factory wykorzystuje dawną pustą lukę, LittleFS i NVS
pozostają w swoich miejscach. Zachowaj build/streamoo.elf: asercje nadal
abortują, ale oszczędzają miejsce przez pominięcie rozbudowanych opisów.

## Najpierw publiczny HE-AAC

W edytorze WWW dodaj osobno:

- Kiss FM: `https://streaming.g-news.pl:8090/kissfm` — w sprawdzonej próbce
  ffprobe HE-AAC. Na S3 profil i odsłuch jeszcze do potwierdzenia.
- Radio Nowy Świat AAC+: `https://stream.nowyswiat.online/aac` — oficjalny
  publiczny wariant AAC+. Dostępność/profil może się zmienić; UI wyświetla AAC.

Sprawdź czyste stereo, prawidłowe tempo/wysokość dźwięku, PLAY/STOP,
głośność i zmianę na znaną MP3/AAC-LC oraz powrót. Przyślij log playera,
przyczyny retry, heap i stack_hwm po kilku minutach. Profilu HE-AACv2 nie
uznajemy za odebrany wyłącznie na podstawie działającego HE-AAC.

## Powtarzalne FLAC/Vorbis/Opus i playlisty

Na komputerze z FFmpeg/ffprobe w tej samej sieci, podstaw jego adres LAN:

```sh
python3 tools/generate_extended_fixtures.py --output-dir /tmp/streamoo-format-test --base-url http://192.168.1.20:8000
python3 -m http.server 8000 --bind 0.0.0.0 --directory /tmp/streamoo-format-test
```

Katalog musi być nowy. Generator niczego nie nadpisuje. Zawiera manifest
kodeków/SHA,7 plików audio i playlist.csv (3 pola TAB zgodne z StreamOO).
Eksportuj bieżącą playlistę radia przed importem listy testowej albo dodaj
adresy testów ręcznie przez WWW. Nie trzeba zmieniać prywatnego pliku
projektu ani wgrywać obrazu filesystemu. Komputer musi być osiągalny
z radia na porcie8000. Agent nie uruchamia tego serwera na Twoim LAN.

Sygnał32s powtarza sekwencję4s: cisza, lewy1kHz, cisza, prawy2kHz,
oba1kHz, cisza; łagodne rampy50ms, peak źródła1036/32768. Po32s jest EOF,
więc ponowna próba playera jest spodziewana. Cisza w sekwencji jest celowa.

Pozycje M3U i PLS mają grać odpowiednio FLAC i Vorbis, nested M3U→PLS
również Vorbis. W PLS File2 jest wcześniej w pliku, ale wybieramy File1.
Opcjonalny `/reject-hls.m3u8` ma dać błąd NOT_SUPPORTED, bez odtwarzania
segmentów. Nie jest dodany do normalnej listy testowej.

## Granice obecnego zakresu

Mono/stereo8..48kHz, PCM16/24/32; scratch64KiB. FLAC natywny, Vorbis/Opus
w Ogg. Większe ramki,>48kHz, multichannel, Ogg-FLAC/chained Ogg i surowy
Opus/RTP nie są deklarowane. HLS osobno OPEN. M3U/PLS to URL pojedynczej
stacji, nie zastępstwo importu katalogu CSV.

Potwierdzenie hostowych testów/build nie zastępuje odsłuchu na ESP32.
Dla każdego formatu zapisz wynik, sample rate, brak underrun/invalid,
stack HWM i minimum heap; sprawdź współpracę FFT oraz reconnect/STOP.

Dla istniejącego sdkconfig z innego checkoutu ustaw w menuconfig → Compiler
options optymalizację Size (-Os) i Assertions → Silent (kontrole włączone).
Nowy sdkconfig bierze te ustawienia z sdkconfig.defaults; sam build nie
nadpisuje istniejących wyborów. W bieżącym workspace są już ustawione.

## Poprawka live FLAC — ADR0056

Radio Mast Raw FLAC zaczyna od ramki0xFFF8 bez fLaC/STREAMINFO. Pierwszy
build06k tego nie obsługiwał: log użytkownika pokazał vendor=-7 po257B.
Nowy adapter sprawdza pierwszy nagłówek/CRC8 i uzupełnia STREAMINFO przed
parserem. Po zwykłym flash wybierz ponownie TEST FLAC native lossless.
Oczekiwany log: `FLAC live frame: supplied validated STREAMINFO`, potem
PCM/PLAYING; rzeczywisty odbiór poprawki nadal OPEN. Nie trzeba zmieniać
playlisty, tablicy partycji ani ustawień. RNŚAAC+ sprawdzamy oddzielnie:
log FLAC nie potwierdza przyczyny jego problemu.

## RNŚAAC+ TLS — ADR0057

Log użytkownika pokazał brak zaufanego root przed HTTP, bez uruchomienia
AAC. Origin serwera podaje chain cross-signed, podczas gdy właściwy GTS
Root R4 już jest w pełnym bundle. Włączono natywne cross-signed verify
ESP-IDF. Bez nowych CA, wyłączania walidacji lub zmiany URL. Po zwykłym
flash sprawdź ten sam RNŚAAC+ i log Certificate validated/HTTP/PCM. Nie
trzeba importować stacji ponownie. Odbiór handshake/dekodera S3 OPEN.

ADR0058: po starcie FLAC występowały przerwy — parser miał fallback20KiB,
a zmierzone ramki24-bit stereo dochodzą do25839B. Teraz okno parsera
bootstrap128KiB i PCM128KiB PSRAM. Po zwykłym flash sprawdzić kilka minut,
w tym zmianę utworu; parser powinien pokazać max_frame131072. Wcześniejsze
opisy scratch64KiB w tej instrukcji zastępuje nowy limit128KiB.

## HLS — ADR0060

Wcześniejsze jawne odrzucanie HLS zastępuje reader nieszyfrowanego live/VOD:
master/media, AAC/MP3 packed audio oraz MPEG-TS przez istniejący codec2.6.2.
Nie obejmuje AES/DRM/fMP4/BYTERANGE/LL-HLS; discontinuity po starcie kończy
sesję/retry. Limity16KiB/32 segmenty, osobny manifest PSRAM, reader16KiB,
TLS i ring256KiB zachowane. Testy hostowe/publiczne próbki PASS, odsłuch
S3 OPEN. Instrukcja: [HLS_BRINGUP](HLS_BRINGUP.md), `playlist.hls.csv`.
Generator extended fixtures dodaje3 lokalne HLS: AAC-TS/MP3-TS/AAC-packed.

## Live FLAC po kolejnym logu — ADR0061

Needed138240 przekroczyło scratch128KiB. Live headerless FLAC teraz
rozdzielane CRC8/CRC16 na pojedyncze ramki, SDK use_frame_dec=true, bez
agregującego parsera/STREAMINFO bootstrap. File fLaC bez zmian. Nowy cache
128KiB+16 PSRAM, PCM scratch nadal128KiB. Po zwykłym flash sprawdzić tę
samą stację kilka minut/zmiana utworu: log `FLAC live: CRC-checked single
frames, parser aggregation disabled`, brak vendor-8 i stabilne low/starts.
Raw decoder SDK i transport przy compressed1 nadal do odbioru S3; hostowe
testy/oracle i build nie są odsłuchem. Szczegóły ADR0061 i STATUS.
