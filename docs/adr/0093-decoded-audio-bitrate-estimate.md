# ADR0093 — bitrate bez deklaracji serwera

Data: 2026-11-01. Status: host PASS; odbiór OLED na S3 OPEN.

Użytkownik zgłosił brak bitrate FLAC z LAN. Dotąd ADR0049/0067 korzystały
tylko z icy-br lub deklaracji HLS; Python http.server ich nie podaje.

## Decyzja

Deklaracja serwera nadal ma pierwszeństwo. Przy jej braku player oblicza:

`kbps = compressed_delta * 8 * rate * channels * (bits/8) / pcm_bytes_delta / 1000`.

To przyrost bajtów zużytych przez dekoder podzielony przez czas źródłowego
PCM, przed resamplingiem i volume. Nie szybkość HTTP/Wi-Fi, rozmiar bufora
ani bitrate PCM. Pierwszy pakiet PCM ustala bazę, wykluczając początkowe
metadane. Publikacja po>=2s następnego PCM, zaokrąglenie do1kbps.
WAIT zachowuje wynik, kolejne okna śledzą VBR. Lookahead, kontener lub
recovery mogą wpływać na oszacowanie; nie deklarujemy dokładności ramki.

Jawny znacznik bitrate_measured w snapshotach daje OLED np. `~1839k`.
Deklarowany bitrate bez tyldy, nieznany nadal --. Znacznik czyścimy po
STOP/retry/offline, zmianie sesji i nieważnych danych stacji/generacji.
Renderer tylko formatuje snapshot. HTTP info nadal opisuje deklarację
nadawcy; preroll/refill nie korzysta z nowego pomiaru wyświetlacza.

Helper streamoo_bitrate.h działa tylko w ownerze dekodera, bez alokacji,
I/O lub nowych zależności. Reset po zmianie formatu/cofnięciu liczników,
kontrola przepełnień i zakresu. Liczniki wspólne dla kodeków; dekodery,
bufory, gain i I2S bez zmian. Nie kopiowano kodu bibliotek.

## Sprawdzenie

Rdzeń: mono/stereo,16/24/32bit,8/44,1/48kHz, prefiks metadanych, okna2s,
WAIT/VBR, reset formatu/liczników, nieważne wejście i duże delty64bit.
Prawdziwy actor:192kbps bez nagłówka, deklarowane320kbps przed pomiarem
192kbps, czyszczenie STOP. UI: tylda zmienia tylko pole bitrate, skrajne
wartości nie wychodzą poza bounds, nieważne audio usuwa znacznik.
Pełne testy i native build w STATUS.

Odbiór: po kilku sekundach LAN16/24/headerless mają pokazywać około
~213k/~1839k, z tolerancją okna i ramkowania. Publiczna stacja z icy-br
nadal pokazuje deklarację nadawcy. Agent nie flashował.
