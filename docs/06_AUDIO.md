# 06 — Audio, dekodery, I²S, player state

**USTALONE:** PCM5102A, ESP-IDF, MP3/AAC jako podstawa oraz docelowa obsługa FLAC, Ogg Vorbis i Ogg Opus, stosowanie optymalizacji ESP32-S3 tam gdzie zweryfikowane. Preferowane biblioteki Espressif, ale wybór konkretnej wersji/dekodera wymaga PoC, benchmarków i przeglądu licencji.

## Pipeline
Network reader (HTTP/HTTPS + icy/redirects) → compressed ring w PSRAM (~256 KiB na start) → demux/parser kontenera → dekoder → PCM L/R → tap przed głośnością → gain ramp → wyjściowy PCM FIFO → I²S DMA → PCM5102A.

## Stany
Odrębne `network_state`, `player_state`, `ui_state`. Odtwarzacz: `IDLE`, `CONNECTING`, `BUFFERING`, `PLAYING`, `STOPPED`, `RECONNECTING`, `ERROR_UNSUPPORTED`. `PLAY/STOP` **nie jest time-shift pause**.

- Uruchomienie: odczyt ostatniej stacji i konfiguracji, wifi, ponowne odtworzenie zgodnie z polityką auto-play (**OTWARTE**: zawsze czy uwzględnia zapisany STOP). SNTP równolegle; HTTPS musi czekać na prawidłowe zaufanie/czas jeśli potrzebuje.
- Lista stacji: stara stacja gra aż do zatwierdzenia.
- Zmiana: generacja żądania, odwołaj stare connect/read, wyczyść bufory i stare metadane, połącz, buforuj, odtwarzaj; stare callbacki nie mogą zmienić stanu.
- Underrun: łagodna rampa ciszy, `BUFFERING`, po odtworzeniu zapasu danych płynny powrót; nie rozłączaj TCP automatycznie przy krótkiej przerwie.
- Retry: 1s, 2s, 4s, 8s, potem maks. 15s; bez sztywnego limitu przy przejściowym niedostępie. Nie zapętlaj się na błędach trwałych typu unsupported codec.
- Nie zmieniaj samodzielnie stacji na inną podczas awarii.

## Bufory — PROPONOWANE
Compressed 256 KiB, start/rebuffer ~64 KiB (adaptowane do bitrate i czasu), PCM 32–64 KiB, I²S DMA w RAM odpowiednim dla DMA. Oblicz czas bufora dla MP3/AAC/FLAC; nie zakładaj stałego opóźnienia.

## Volume
- Skala 0..100, 0 mute, 100 unity (0 dB) i brak cyfrowego przesterowania.
- Proponowana zależność dla v>0: `gain = 10^((60*(v/100.0-1))/20)`; v=0 gain=0. Zmiany wzmocnienia rampuj w dziedzinie próbek, 30–100ms do strojenia.
- Nie stosuj zniekształcającego zakres dwuczęściowego mapowania; nie przeprowadzaj FFT po tłumieniu sygnału.

## Testy
MP3/AAC LC/HE-AAC oraz FLAC/Vorbis/Opus w wspieranych kontenerach, różne sample rates, URL redirect/TLS/ICY, utrata Wi-Fi, przepełnienie/niedopełnienie, szybkie zmiany stacji, brak drop-out, obciążenie CPU oraz heap i stabilność wielogodzinna.

## Część 06a — natywne I2S i skończony ton

`audio_output`: I2S0 master 48kHz, stereo Philips 32-bit, slot32, bez MCLK,
GPIO DOUT48/BCLK47/WS21. Wypełnianie DMA w IRAM callback bez blokującego
write, flash, alokacji i FFT. [ADR 0018](adr/0018-i2s-output-tone-bringup.md).

Domyślny STREAMOO_I2S_SMOKE_TEST uruchamia około 10s sekwencji z cichym
sygnałem ~-30dBFS peak i rampami50ms: cisza, lewy1kHz, prawy2kHz, oba1kHz,
z przerwami; potem cisza i wyłączenie kanału. UI/NVS nadal działają osobno.
TEST VOL/PLAY nie sterują tym sygnałem. DAC musi sprzętowo mieć ustawiony
format I2S, być odmutowany i obsługiwać PLL bez MCLK; model/zwory użytkownika
nie są jeszcze potwierdzone. Sterownik, zegary i dźwięk wymagają odbioru.
Pełny pipeline, volume gain, dekodery i odtwarzanie strumieni nadal OPEN.

## Część 06b — rzeczywisty gain i PLAY/STOP na tonie

Nowy domyślny STREAMOO_AUDIO_CONTROL_TEST uruchamia sterowany ciągły ton
1kHz stereo zamiast sekwencji 06a. Start w STOP z volume z NVS. Enkoder
wysyła komendy do osobnego właściciela, a UI odczytuje jego snapshot.
Gain 0..100 i PLAY/STOP mają rampę50ms w próbkach; VOL0 mute, VOL100 unity.
Źródło ma peak~-30dBFS przed gain, więc przy niskim volume będzie bardzo ciche.
[ADR0019](adr/0019-controlled-audio-gain-ramp.md) opisuje Q16, IRAM, komendy
i aktualizacje NVS. Widżety/katalog są nadal syntetyczne. STOP podaje zera
bez wyłączania zegarów; nie jest pauzą odtwarzanego strumienia. Polityka
autoplay rzeczywistego radia pozostaje OPEN. Pełny pipeline nadal niegotowy.

## Część 06c — raw PCM FIFO i tap

Źródło testowe napełnia FIFO w zadaniu; callback I2S już nie generuje tonu.
Obecny wariant: raw PCM FIFO → tap → gain → DMA (uzasadnienie położenia
FIFO w [ADR0020](adr/0020-pcm-fifo-and-pre-gain-tap.md)). Pojemność źródła
160ms, rampa ochronna przy rezerwie50ms, restart po odbudowaniu120ms.
Brak danych daje ciszę, nie restart sieci (sieć nadal niezaimplementowana).
Tap ma osobny odbiór i odrzuca nowe bloki przy przeciążeniu; numery sekwencji
pozwalają wykrywać luki. Tylko debug peak, bez FFT/RMS i rzeczywistych widżetów.
Opcjonalne eksperymenty stall/tap-delay opisane w HARDWARE_BRINGUP.
Format jest na razie stały stereo32/48kHz; dekodery/generacje źródeł OPEN.

## Część06d — MP3 z wbudowanego pliku

Dokładnie przypięty esp_audio_codec2.6.2 z licencją Espressif Modified MIT
(ESP-only) i notice Apache dla MP3. [ADR0021](adr/0021-mp3-decoder-file-poc.md).
Nowy domyślny MP3_FIXTURE_TEST dekoduje przed I2S4s oryginalny plik do
PSRAM; producent konwertuje PCM16LE do FIFO stereo32/48kHz. Brak dekodera,
PSRAM i odczytu flash w ISR. Batch predecode nie jest streamingiem/benchmarkiem
realtime. Wymaga odbioru logu MP3 PoC i odsłuchu na ESP32-S3.

Start nadal STOP, volume z NVS; PLAY odtwarza powtarzaną sekwencję kanałów
z wbudowanymi przerwami. Widżety/katalog TEST. Peak tap zmienny, nie stałe1036.
Adapter odrzuca inne rate/bits/channels niż wspierane, bez resamplingu.
W06e wybór źródła zastępuje dawny przełącznik MP3.

## Część06e — AAC-LC ADTS

Wspólny adapter MP3/AAC-LC, bez zmiany wersji zależności. AAC-Plus jawnie
wyłączony; HE-AAC/SBR/PS OPEN. Domyślny wybór źródła to AAC-LC, alternatywy
MP3 i sinus. Batch decode przed I2S, ten sam adapter PCM/FIFO/tap/gain.
[ADR0022](adr/0022-aac-lc-adts-file-poc.md). Build trzech wariantów PASS,
odbiór AAC na sprzęcie OPEN. Następny krok po odbiorze: streaming decode
z backpressure; aktualny PoC nie mierzy deadline dekodera czasu rzeczywistego.

## Część06f — decode podczas odtwarzania

Pełny PCM klipu zastąpiony małym buforem roboczym: wejście257B, PCM8192B,
ogony wejścia/wyjścia i pull240 ramek. Decoder pozostaje otwarty w zadaniu,
pełne FIFO zatrzymuje decode; EOF resetuje parser/dekoder. Początkowy zapas
FIFO przed I2S, potem praca podczas DMA. ISR/gain/tap bez zmian.
[ADR0023](adr/0023-incremental-file-decode.md). Źródło nadal we flash,
bez sieci/resamplingu/HE-AAC. AAC06f odebrane logiem i odsłuchem (STATUS).

## Część06g — źródło skompresowane i backpressure

Pump odczytuje callback źródła zamiast pliku: DATA/WAIT/END/ERROR i last.
Pierścień nie nadpisuje danych, częściowy zapis wstrzymuje feeder; ostatni
bajt zatrzymany do finish zapewnia EOS na rzeczywistym wejściu. WAIT zachowuje
częściowy blok PCM i nie publikuje go do FIFO. Nowy bufor testowy4096B PSRAM,
feeder997B; to eksperyment zawijania, nie produkcyjny bufor256KiB.
Wszystkie operacje źródła w jednym zadaniu; wielozadaniowy reader wymaga
adaptera synchronizacji. [ADR0024](adr/0024-compressed-source-buffer.md).
ISR/gain/tap bez zmian. Odbiór06g na S3 OPEN, sieć nadal niezaimplementowana.
