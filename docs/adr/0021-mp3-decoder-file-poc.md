# ADR 0021 — MP3 z pliku i adapter PCM

Data: 2026-10-25. Status: przyjęty dla06d; dekodowanie/odsłuch na sprzęcie OPEN.

## Wybór i licencja

Wybrano espressif/esp_audio_codec **2.6.2**, dokładny constraint w manifeście
decoder i hash w dependencies.lock. Rejestr i pakiet deklarują IDF>=4.4,
zawierają biblioteki ESP32-S3. To nie dowód działania na6.1: wykonano nasz
build/link ESP-IDF6.1; runtime wymaga płytki. Pakiet ma gotowe archiwa,
nie budujemy algorytmu dekodera ze źródeł. API/parser/wrapper są oddzielne
od Arduino i od monolitycznego ADF; nie używamy pipeline/API yoRadio.

Licencję dokładnie tej wersji sprawdzono przed integracją. Espressif Modified
MIT ogranicza użycie do sprzętu Espressif i wymaga zachowania notice; pasuje
do zadeklarowanego celu ESP32-S3. To nie jest zwykłe MIT. Oficjalny wykaz
Espressif wskazuje Apache2.0/Android dla MP3 i AAC. Zachowano licencje/notice
w licenses oraz wpis THIRD_PARTY_NOTICES. Nie importujemy implementacji GPL.
FFmpeg/libmp3lame są tylko narzędziami hosta, produkują oryginalne dane
sinusoidalne; nie kopiujemy ich kodu do targetu. Bez nowych nagrań zewnętrznych.

Źródła:
- https://components.espressif.com/components/espressif/esp_audio_codec/versions/2.6.2
- https://components.espressif.com/components/espressif/esp_audio_codec/versions/2.6.2/license
- https://docs.espressif.com/projects/esp-adf/en/latest/COPYRIGHT.html
- LICENSE/manifest/public headers pakietu z rejestru (commit źródłowy f9110cbb471b4017b0faf9bd48fcac84c61ecb6d).

## API i granice

Komponent decoder opakowuje Simple Decoder z MP3 i use_frame_dec=false,
żeby parser przyjmował fragmenty niepokrywające się z ramkami MPEG.
Rejestrujemy wyłącznie esp_mp3_dec_register, bez rejestracji domyślnej.
Pierwsze open/rejestracja tylko z jednego właściciela dekodera. Osobne
handle, consumed/produced/needed/codec_status, format z get_info po PCM,
sprawdzanie granic deklarowanych rozmiarów i close. Wywołania/allocacje
wyłącznie w zadaniu; nie w callbacku I2S. Brak hostowej implementacji vendor;
unit testy natywne nie wykonują tych archiwów.

Własny pcm_adapter przyjmuje 48kHz signed16LE, mono/stereo, do240 ramek.
Mono duplikuje doL/R, signed32 powstaje przez mnożenie65536 bez UB signed
shift. Byte loads działają na niewyrównanym wejściu. Inne rate/bits/channels
są odrzucane przed zmianą wyjścia; brak udawanego resamplingu. Pętla PCM
ma jawnego właściciela kursora i pożyczone, niezmienne dane; input/output
nie mogą się nakładać. Zawijanie może wypaść wewnątrz bloku240.

## PoC pliku i ścieżka danych

Nowy domyślny STREAMOO_MP3_FIXTURE_TEST wymaga sterowanego testu audio.
Przed startem I2S zadanie dekoduje oryginalny4s plik MP3 ze flash do bufora
1MiB w PSRAM, fragmentami po257B. Scratch8192B jest tymczasowy w internal
RAM, stos zadania24576B. Po dekodowaniu handle/scratch są zwalniane, PCM
pozostaje do zapętlania. Błędy nie włączają zastępczego tonu: serwis kończy
z błędem, a UI nie potwierdza PLAY bez gotowego właściciela. Błąd później
zwalnia również klip. Przygotowanie yield co32 wywołania, żeby nie blokować
niższych priorytetów przez dłuższy fragment pracy.

Fixture64512B: MPEG1 LayerIII/CBR128k, stereo48kHz. Źródło192000 ramek,
referencja FFmpeg193536 (168 ramek MPEG). PCM innego dekodera może mieć
inną politykę priming/padding; PoC wymaga192000..194688 ramek, poprawnego
formatu oraz peak0<peak<=2048. To kontrola tej cichej próbki, nie limit
zakresu ogólnego dekodera. Liczby/metadane/timing są logowane. Brak progresu,
przepełnienie lub >10000 wywołań przerywa próbę zamiast pętli bez końca.

Po prepare producent czyta PCM16 z PSRAM, adapter konwertuje do SRAM FIFO,
tap/gain/DMA działają jak06c. ISR nie dotyka pliku, PSRAM ani dekodera.
To **batch predecode**, nie dekodowanie w czasie rzeczywistym: czas decode_us
obejmuje wywołania process, wall_us również przygotowanie i yield. Nie
ogłaszamy jeszcze wydajności playera/streamingu ani akceleracji SIMD MP3.

Klip powtarza ciszę/L1k/R2k/oba1k/ciszę. Start w STOP z volume z NVS,
PLAY wchodzi w aktualne miejsce cyklu. Wbudowane przerwy nie są underrun:
low/hard powinny nie rosnąć w normalnym profilu. Tap peak nie jest teraz
stale1036, bo PCM zawiera ciszę i kompresję stratną. UI pokazuje MP3,
widżety/katalog nadal TEST. Nie dodano sieci, LittleFS ani prawdziwych stacji.

AAC/HE-AAC, FLAC/Vorbis/Opus,44100/mono fixture, resampling, streaming
process/backpressure/EOS/cancel/generation i pełny player pozostają OPEN.
Link map/nm wykazał MP3 open/decode, bez AAC/Opus/FLAC open/decode.
Odtworzenie klipu na sprzęcie jest następną bramą; build nie jest wynikiem dekodera.
