# ADR0060 — HLS audio w istniejącym ownerze HTTP

Data:2026-10-31. Status: implementacja i host PASS, odsłuch ESP32-S3 OPEN.

Użytkownik zlecił punkt8 HLS obok ustawień radia z ADR0059. Własny bounded
parser RFC8216 i reader segmentów rozszerzają istniejący moduł streaming.
Nie kopiujemy implementacji HLS ani nie dodajemy zależności. Dokumentacja:
https://www.rfc-editor.org/rfc/rfc8216.html .

Zakres: nieszyfrowane playlisty media live/VOD, master z audio rendition
lub wariantem audio, AAC/HE-AAC ADTS i MP3 packed audio oraz AAC/MP3 w
MPEG-TS. Preferencja jawnego audio rendition, poza tym najniższy bandwidth
<=512k bez RESOLUTION i znanych kodeków wideo. Bez adaptacyjnego przełączania
jakości w trakcie. TS korzysta z przypiętego esp_audio_codec2.6.2:
`impl/esp_ts_dec.h`, `esp_ts_dec_register`, AACplus=true, rejestracja
podległych MP3 i AAC. Licencja Espressif Modified MIT już zachowana;
API sprawdzone w dokładnych nagłówkach zależności IDF6.1.

Parser16KiB,32 segmenty, target1..120s, master do3 poziomów; każdy zasób
maksymalnie5 redirect, bez HTTPS→HTTP lub credentials. TLS korzysta z
istniejącego pełnego bundle i sprawdzania nazw/podpisów/dat. Packed audio:
usuwanie do4 ID3v2 na początku segmentu, limit16KiB; AAC/MP3/TS sniffing
przed przekazaniem. Segment do4MiB strumieniowany, bez pełnego stagingu.
Zmiana kodeka lub discontinuity po rozpoczęciu kończy sesję i oddaje retry
playerowi. AES/DRM, fMP4/EXT-X-MAP, BYTERANGE, LL-HLS PART, GAP i I-frames
jawnie odrzucone. Nie deklarujemy obsługi tych wariantów ani synchronizacji
z wideo. Dla TS OLED pokazuje kontener MPEG-TS, bez zgadywania kodeka.

Live start od ostatnich3 segmentów; monotoniczny MEDIA-SEQUENCE pomija
już odtworzone. Utracone okno kończy sesję zamiast powtórzyć stare audio.
Reload po połowie target;6 kolejnych reloadów bez postępu daje timeout.
ENDLIST kończy źródło, zgodnie z dotychczasowym EOF/retry. STOP przerywa
oczekiwanie reload w100ms, blokujące HTTP nadal w osobnym ownerze/ACK.

Dokument/manifest około84KiB PSRAM poza ringiem256KiB (reload nie może
nadpisać audio). Reader stack16KiB zamiast10KiB na parsowanie/TLS; PCM
scratch128KiB i brak flash/alloc/I/O/FFT w I2S bez zmian. Heap/HWM/CPU/soak
S3 wymagają pomiaru. Nie zmieniono GPIO, partycji, NVS i prywatnej playlisty.

368 testów hostowych PASS. Realny C reader na fake HTTP: master, packed
ID3, TS, live reload/no replay, STOP/cleanup i błędy. Parser bounds20k,
ASan/UBSan dla reader/parser/decoder PASS, LSan off; nie są testem binarnego
dekodera na S3. Native ESP-IDF6.1 PASS, obraz0x17a440 (90% app wolne).
Generator tworzy oryginalne32s AAC-TS/MP3-TS/AAC-packed; FFprobe i nasz
parser potwierdziły3 VOD. Sześć publicznych RadioMast HLS: rzeczywiste
manifesty i po jednym segmencie/FFprobe PASS na hoście. `playlist.hls.csv`
i raport ignorowaneGit, oryginalny playlist.csv nietknięty. Odbiór PCB
HLS_BRINGUP; agent nie wykonywał flash/erase/eFuse.
