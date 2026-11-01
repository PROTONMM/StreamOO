# ADR0077 — przyrostowy parser ramek FLAC przez publiczne API SDK

Data:2026-11-01. Status: host/build PASS; kontrakt binarnego SDK i odsłuch S3 OPEN.

## Dowody i granica diagnozy

Log użytkownika6e9a0e50 po ADR0076: ten sam utwór mocno przerywa.
Gen8: wait_empty139, wait_busy2, compressed1, low16; potem vendor-8,
needed1050624 > scratch1048576. To38×27648B, podczas gdy poprawne wyjścia
mają pcm_max27648B (4608 próbek, stereo24). Samo podnoszenie scratch nie
rozwiązało problemu. W gen8 najdłuższe wywołanie kodeka264367us, blok301346us,
ring_at_max24/0. Dziesięciosekundowe obciążenie rdzeni około46%/40%, FFT
avg3054us/max6393us. Brak dowodu na stałe przeciążenie CPU albo główną rolę
mutexu; średnie nie wykluczają lokalnych opóźnień.

WAIT przy empty oznacza pusty ring pomiędzy readerem a pump, nie dowodzi
samodzielnie awarii Wi-Fi. Parser też może gromadzić dane zamiast wydawać
PCM na bieżąco. Z logu nie można potwierdzić wszystkich przyczyn niedoborów.
Wielokrotność27648 sugeruje grupowanie/powtarzanie wyników, ale nie jest
samodzielnym dowodem błędu wewnątrz binarnej biblioteki.

W ELF wcześniejszego builda sprawdzono esp_flac_dec_parse_frame: STREAMINFO
max_frame128KiB wyznacza okno max_frame_size128KiB+18B. Potwierdza to log
AUD_Dec_Parse max_frame131090. W publicznym esp_es_parse_types.h2.6.2
opisano dwie strategie: oczekiwanie na max_frame_size albo
ES_PARSE_REPARSE_FRAME_SIZE, czyli ponowne parsowanie po nadejściu danych.
Nie dowiedziono na hoście pełnego przebiegu binarnego SDK; nowy wariant ma
usunąć zależność wydawania ramek od zapełnienia dużego okna.

## Decyzja

Własny parser granic przez publiczne esp_audio_simple_dec_register, z tymi
samymi ESP_FLAC_DEC_DEFAULT_OPS. Pozostają esp_audio_simple_dec, jego cache,
zwykły tryb use_frame_dec=false i dotychczasowy bootstrap STREAMINFO.
Nie przywracamy odrzuconej ścieżki raw z ADR0061: nie ma przełączania handle,
dodatkowego128KiB cache ani bezpośredniego przekazywania ramek do raw API.

Callback ustawia REPARSE, utrzymuje przyrostowy CRC16 i indeks w extra_data,
oddaje pierwszą kompletną ramkę po CRC16 oraz CRC8 następnego nagłówka.
Po EOF akceptuje ostatnią kompletną ramkę z prawidłowym CRC16. Dwa poprawne
pakiety mogą łącznie mieć poprawne CRC16, więc sam CRC całego okna nie
wystarcza: pierwsza poprawna granica ma pierwszeństwo także przy EOF.
Nagłówki kolejnych ramek mogą odwoływać się do STREAMINFO. Zwykłe pliki
fLaC obsługiwane tą samą ścieżką; dalsze metadane, także duża grafika,
pomijane porcjami. Limit pojedynczej ramki nadal128KiB plus16B lookahead.

Stan kilkudziesięciu bajtów alokowany przez calloc przy inicjalizacji
parsera, zwalniany przez SDK zgodnie z udokumentowanym kontraktem free=NULL.
CRC tabela16 wartości wyliczona z wielomianu0x8005, kod własny; bez importu
implementacji FLAC ani nowej zależności. Wersja nadal esp_audio_codec2.6.2,
licencja Espressif Modified MIT (ESP-only), API i nagłówki zweryfikowane.
Punkty odniesienia: [rejestracja parsera](https://github.com/espressif/esp-adf-libs/blob/master/esp_audio_codec/include/simple_dec/esp_audio_simple_dec_reg.h),
[kontrakt ES parsera](https://github.com/espressif/esp-adf-libs/blob/master/esp_audio_codec/include/simple_dec/esp_es_parse_types.h).
Do implementacji użyto przypiętych lokalnych nagłówków2.6.2, nie ruchomego master.

Scratch1MiB, prebuffer/refill, transport, PCM FIFO, zadania, DSP i I2S nie
zmieniają się w tym eksperymencie. Nie przenosimy parsera/CRC/alokacji do I2S.

## Weryfikacja i odbiór

403 testy hostowe PASS. Oracle FFmpeg/FFprobe: granice i bajty każdej ramki
ciszy (ramki<64B), tonu i szumu24-bit (>20KiB), porcje1/7/257/4096/65536B,
CRC uszkodzony, ucięty EOF, limit, bootstrap, metadane200000B. Test wielkiej
paczki poprawnych ramek przy EOF oddaje pierwszą, nie całe okno. Native
ASan/UBSan: header/parser14 testów i adapter2 testy PASS. Realny wcześniejszy
capture RadioMast:178 pełnych ramek,12837..25839B, granice zgodne z FFprobe;
ostatni ucięty pakiet pominięty tylko w tym niezależnym porównaniu.

Native ESP-IDF6.1 build build/simd-check PASS0x17e810,90% factory wolne;
walidatory sdkconfig/defaults i diff --check PASS. Host nie uruchamia
binarnego dekodera Xtensa: potwierdzenie publicznej rejestracji/cache/reset
na S3 oraz stabilnego odsłuchu pozostaje OPEN. Nie deklarujemy naprawy
wszystkich niedoborów transportu na podstawie testów parsera.

Zwykły build/flash bez erase, zachować NVS/LittleFS/playlisty. Ten sam TEST
FLAC native lossless co najmniej5min i zmiana utworu. Log początkowy:
`FLAC live frame: supplied validated STREAMINFO; incremental CRC frame parser`.
Zebrać Decode perf/diag, Underrun, phase i CPU. Sprawdzić brak nowego
vendor-8/needed1050624, pcm_max, maksymalny czas kodeka i przyrosty low/starts.
Potem MP3/AAC i powrót na FLAC, STOP/PLAY/reset. Przy błędzie rejestracji lub
dekodera zachować cały log; nie zwiększać ponownie scratch bez nowego pomiaru.
Agent nie flashował, nie kasował flash i nie zmieniał prywatnych playlist.
