# ADR0065 — zachowanie parametrów sesji przy odświeżaniu HLS

Data:2026-10-31. Status: przyczyna odtworzona, host/build PASS, odsłuch poprawki S3 OPEN.

Użytkownik wskazał TEST HLS128k-mp3-stereo. Log: gen12 MP3/44100/stereo16,
low0/hard0/starts1, po chwili stage=http_end, HTTP200, ESP_ERR_NOT_SUPPORTED,
fetched590577/produced6511104, następnie reconnect. To nie ten sam przypadek
co FLAC. Zdekodowany PCM odpowiada około36,91s, zgodnie z trzema początkowymi
segmentami po12,3037s. Początkowe granie HLS na S3 potwierdzone; reload nie.

## Przyczyna potwierdzona

Pinned ESP-IDF6.1, commitfff9895c82d744c7237be8847347bdd1b07c6643:
`components/esp_http_client/esp_http_client.c`, `esp_http_client_get_url`
(około2212) rekonstruuje scheme/host/port/path, pomijając query. `set_url`
przechowuje path i query osobno (około1320); nie kopiujemy kodu SDK.

Reader po master pobierał pełny media URL z `?lsid=...`, ale do następnego
reload zapisywał wynik tego gettera. Publiczny serwer RadioMast sprawdzony
na komputerze: zlsid daje MEDIA, ten sam host/path bez query daje MASTER
z nowym lsid. HLS owner po rozpoczęciu celowo odrzuca ponowne wejście
w MASTER, stąd HTTP200 + NOT_SUPPORTED po trzech segmentach. Nie był to
błąd samego dekodera MP3 ani dowód na niedostateczny dopływ PCM.

Pobrano manifesty i5 segmentów źródła publicznego; ffprobe potwierdził
MP3/44100/stereo. Początek segmentów: ID3v2.4 z timestampem oraz osobny
ID3 z tytułem, potem MP3. Dotychczasowy reader poprawnie obsługuje te dwa
tagi; nie zmieniamy limitów ID3 ani obsługi discontinuity. Materiał audio
pozostaje w/tmp i nie jest dodawany do repo. Prywatne playlisty bez zmian.

## Poprawka

Reader utrzymuje własny pełny request_url, aktualizowany dopiero po
udanym set_url. HLS cache media URL, M3U/PLS i redirect używają tej wartości,
bez lossy get_url i bez odczytu prywatnej struktury klienta SDK. Parametry
sesji i podpisów, kodowanie%2F i kolejność query nie są dekodowane/zmieniane.

Location jest przechowywane do zakończenia nagłówków, następnie rozwiązywane
względem pełnego bieżącego URL przez istniejący resolver. Dotyczy też
Location zaczynającego się od `?`. Odrzucone puste, zbyt długie i podwójne
Location, credentials, schematy inne niżHTTP(S) i HTTPS→HTTP. Limity2048B,
5 redirect na zasób i ograniczenia HLS zachowane. Nie opieramy sprawdzenia
TLS downgrade na getterze pomijającym query. SDK/certyfikaty/hostname/date
verification bez zmian; tracker nie dodaje logowania adresów/query ani
osobnego zapisu w NVS.

Dwa bufory2049B w istniejącym obiekcie PSRAM, około4KiB dodatkowej pamięci;
bez alokacji w I2S, nowych tasków, zmian ring/PCM/dekodera/gain. Brak nowych
zależności/licencji, GPIO/partycje i API użytkownika bez zmian. Wgrana lista
nadal może zawierać oryginalny masterURL; nie trzeba jej importować ponownie.

## Weryfikacja

Mock get_url poprawiony, aby odzwierciedlał rzeczywiste pominięcie query
w pinned SDK; poprzednio fałszywie zwracał pełny URL. Stary reader odtworzył
NOT_SUPPORTED na pierwszym reload po zgubieniu lsid. Nowy przechodzi trzy
segmenty/dwa reload, przesuwające się okno bez replay, redirect query-only,
podpisany URL segmentu i odrzucenie downgrade. Fixture ma dwa początkowe
ID3, jak w prawdziwych segmentach. Wcześniejsze HTTP/ICY/M3U/PLS/redirect,
błędy/STOP/cleanup zachowane. Natywny getter ani set_redirection nie są już
używane przez reader; set_url i własny resolver wystarczają.

374 testy hostowe PASS, HTTP ASan/UBSan PASS (LSan off), native ESP-IDF6.1
build PASS0x17aa40,90% app wolne, GPIO/diff PASS. Atrapy transportu nie są
odsłuchem S3. Hostowa odpowiedź serwera z query i bez niego sprawdzona live.

Odbiór: zwykły flash bez erase/FS/reimport, TEST HLS128k-mp3-stereo co
najmniej3minuty, przez wiele reload. Oczekiwane jedna generation, low0,
starts1 i brak http_end/NOT_SUPPORTED po~37s. Nadal możliwe inne jawnie
nieobsługiwane warianty HLS z ADR0060; nie obiecujemy AES/fMP4/discontinuity.
FLAC nadal osobno OPEN — potrzebne Decode perf z ADR0064. Agent nie wykonał
flash/erase/eFuse ani zmian prywatnego playlist.csv.
