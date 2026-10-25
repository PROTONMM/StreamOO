# ADR0022 — AAC-LC ADTS przed I2S

Data:2026-10-25. Status: wdrożone; odbiór sprzętowy AAC OPEN.

## Kontekst

MP3 decode/PCM działa na ESP32-S3 według logu użytkownika. Użytkownik
wyjaśnił nieczytelny fragment UART poruszeniem kabla i polecił kontynuować.
Sprawdzamy drugi podstawowy kodek przed pracą nad streaming decode.

## Decyzja

Wspólny adapter Simple Decoder dla MP3 i AAC-LC/ADTS, dokładnie przypięty
esp_audio_codec2.6.2. API AAC sprawdzone w publicznym esp_aac_dec.h pakietu:
no_adts_header=false, channel=2, aac_plus_enable=false. Konfiguracja żyje
razem z handle. Rejestrujemy tylko wybrany backend, bez register_default.
Warianty plikowe linkują MP3 i AAC; sinus nie potrzebuje kodu dekoderów.
Adapter jest przeznaczony do jednego właściciela w zadaniu, nie do ISR.

Kconfig wybiera AAC-LC (domyślnie), MP3 lub sinus. Własny klip ADTS37039B,
189 ramek po1024 próbki,48kHz stereo. FFmpeg na hoście tworzy dane z tego
samego matematycznego źródła co MP3; nie kopiujemy/linkujemy jego kodu.
Nie zakładamy CBR mimo celu128kbps. SHA/narzędzie/referencja w JSON.

Zachowujemy batch predecode przed I2S, chunk257B, scratch8KiB, limit1MiB
PSRAM, stos24KiB i kontrolę formatu/progresu/peak. Po decode zamykamy handle;
producent konwertuje PCM16LE do istniejącego FIFO stereo32 w SRAM. Bez zmian
w ISR, gain, tap i rampach. Start STOP, preferencje NVS zachowane.

## Licencje i ograniczenia

Licencja komponentu Espressif Modified MIT ograniczona do produktów Espressif.
Oficjalny COPYRIGHT Espressif wskazuje Android Open Source Project2005–2008,
Apache2.0 dla MP3 i AAC. Notices zachowane/uzupełnione w licenses i root notice.
Źródła: przypięty pakiet LICENSE/publiczne nagłówki oraz
https://docs.espressif.com/projects/esp-adf/en/latest/COPYRIGHT.html

Nie dowodzi to HE-AAC/SBR/PS, innych kontenerów, resamplingu, dekodowania
realtime, benchmarku obciążenia ani pracy sieci. Nie ogłaszamy ich obsługi.

## Weryfikacja i następny krok

142 testy hostowe PASS, w tym integralność/nagłówki189 ramek AAC. Native
ESP-IDF6.1 build/link PASS: MP3 0x87400B, sinus0x4acd0B, AAC0x808b0B.
Końcowy lokalny obraz AAC, stress n/0. Nie flashowano urządzenia.
Odbiór: AAC-LC PoC ESP_OK, poprawny odsłuch L/R/oba kilku cykli,
PLAY/STOP/VOL i stabilne PCM low0/hard0/invalid0. Potem streaming decode
z backpressure zamiast przechowywania całego PCM.
