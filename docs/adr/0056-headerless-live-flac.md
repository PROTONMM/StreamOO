# ADR0056 — FLAC live bez nagłówka pliku

Data:2026-10-31. Status: poprawka gotowa; odbiór S3 OPEN.

Użytkownik zgłosił brak grania nativeFLAC i RNŚAAC+. Dostarczony log,
potwierdzony jako TEST FLAC native lossless: certyfikat OK, HTTP200,
stage=decode/cause=decoder_error, vendor=-7 (NOT_SUPPORT), już pierwszy
fragment257B, brak PCM. To nie był błąd zestawiania połączenia.
RNŚAAC+ wymaga osobnego logu; jego przyczyny nie wywnioskowano z FLAC.

Ponowny odczyt odpowiedzi z nagłówkami StreamOO: audio/flac, icy-metaint,
początek0xFFF8 — od razu ramka FLAC, bez fLaC/STREAMINFO. Pierwszy adapter
ADR0054 pomijał ograniczenie Simple Decoder2.6.2: README jawnie wymaga
nagłówka pliku FLAC. Stąd poprawne odtwarzanie na ffprobe nie wystarczało.

## Decyzja

Oryginalny, bounded adapter nagłówka według RFC9639 sekcje8.2/9.1.
Zbiera maks16B pierwszego nagłówka (także przy wejściu1B), sprawdza sync,
reserved bits, coded number/canonical extended UTF-8 i CRC8. Z jawnych
pól odczytuje blocksize, rate, channels i bits. Dla nativeFLAC z fLaC nie
uzupełnia metadanych. Dla live ramki dodaje42B fLaC+STREAMINFO i odtwarza
przed parserem oryginalny nagłówek ramki, bez utraty/dublowania jej danych.

Parametry8..48kHz/mono-stereo/16/24/32-bit, w granicach64KiB scratch.
Nie zgaduje parametrów z bitrate/MIME: jeśli frame używa rate/bits tylko
z nieobecnego STREAMINFO, odmawia. Blocksize fixed ze sprawdzonej ramki;
variable ma min16/max ograniczony scratch. Nieznane frame byte sizes,
łączna liczba próbek i MD5 są oznaczone zerami, zgodnie z RFC. Dalsze
parsowanie/dekodowanie i CRC ramki nadal wykonuje dostawca.

Bootstrap i reset mają jawny stan; liczniki consumed odnoszą się do
bajtów źródła, nie wstrzykniętego nagłówka. Brak nowego bufora pełnej
ramki lub alokacji aplikacji w process. Całość w zadaniu dekodera,
I2S/gain/DSP bez zmian. Log: FLAC live frame: supplied validated STREAMINFO.
Dodano numeryczny codec do Stream failed:0MP3,1AAC,2FLAC,3Vorbis,4Opus,
żeby kolejne logi nie wymagały zgadywania rodzaju źródła.

## Weryfikacja

- Rzeczywisty pierwszy header z odpowiedzi serwera: CRC8 i pola sprawdzone.
- Macierz pól, rate extras/block extras, coded number, uszkodzone CRC,
  brak parametrów/reserved/range oraz10000 wejść corpus.
- Niezależny FFmpeg: wygenerowany oryginalny FLAC po usunięciu metadanych
  i naprawie daje identyczny PCM; także po dołączeniu od drugiej ramki.
- Rzeczywisty adapter z publicznymi nagłówkami dostawcy i fake binary calls:
  komplet/1B/reset, przekazanie STREAMINFO+oryginalnego nagłówka dokładnie raz,
  regresje plikowego FLAC i pozostałych kodeków/bounds.
- Host/ASan/UBSan/native build według STATUS. Binarnego dekodera Espressif
  nie wykonano na hoście; potwierdzenie działania poprawki na S3 OPEN.

Źródła (odczytane przed implementacją; nie kopiowano kodu):
- Pinned esp_audio_codec2.6.2 README, tabela Simple Decoder FLAC.
- https://www.rfc-editor.org/rfc/rfc9639.html#section-8.2
- https://www.rfc-editor.org/rfc/rfc9639.html#section-9.1
