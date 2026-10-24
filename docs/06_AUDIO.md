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
