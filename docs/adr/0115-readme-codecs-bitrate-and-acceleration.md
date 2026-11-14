# ADR0115 — kodeki, bitrate i akceleracja w README PL/EN

Data: 2026-11-14. Status: wdrożone na polecenie użytkownika.

## Decyzja

Oba README zawierają zgodne zestawienie kodeków i profili, CBR/VBR,
parametrów PCM/I²S, HTTP/HTTPS i playlist oraz zakresu SIMD i sprzętowej
kryptografii. Rozszerzono także bieżące rozdziały techniczne EN i odsyłacz PL.

Nie deklarujemy niezmierzonych granic bitrate. 128 kb/s MP3/AAC i 96 kb/s
Opus to parametry generatorów próbek; 512 kb/s to filtr `BANDWIDTH` dla
wariantów HLS `EXT-X-STREAM-INF`. Nie są to limity wszystkich strumieni.
Próg 4096 kb/s w metadanych/estymacji także nie oznacza odbioru wydajności.
README wyjaśnia deklarację źródła, estymację ≈ i etykietę „Zmienny”.

SIMD obejmuje kernel FFT ESP-DSP sc16 AES3 i część Opus SILK LPC/PLC.
Pozostałe etapy widma są skalarne; audyt pozostałych dekoderów nie daje
podstaw do deklaracji ich wektoryzacji. Sprzętowe AES i SHA są włączone
w domyślnej konfiguracji TLS, z przygotowaniem zasobów przed I²S.
Zakres zależy od operacji i zestawu szyfrów; AES3 FFT nie jest szyfratorem AES.
HTTPS źródeł audio, lokalny panel HTTP i szyfrowanie segmentów HLS mają
osobno opisany zakres obsługi.

## Źródła i weryfikacja

- Adapter dekodera, README przypiętego esp_audio_codec 2.6.2, parser HLS,
  format live PCM, liczniki bitrate i generatory próbek porównane z opisem.
- Aktualne `sdkconfig.defaults` i wygenerowany nagłówek konfiguracji:
  SIMD, hardware AES, hardware SHA i early crypto init włączone.
- SHA-256 archiwum kodeków zgodny z [audytem SIMD](../CODEC_SIMD_AUDIT.md):
  `dc511e7ac323fd7a32a50ecb022608ec21a2e4eab3b5e075cf7fa1478612293c`.
- Mapa bieżącego ELF zawiera `opus_silk_lpc_analysis_core` i
  `dsps_fft2r_sc16_aes3_`. Disassembly narzędziem `xtensa-esp32s3-elf-objdump`
  potwierdza `ee.vmulas.s16.accx` w pierwszej oraz `ee.cmul.s16`,
  `ee.vmul.s16`, `ee.vadds.s16`, `ee.vsubs.s16` w drugiej funkcji.
- Build ESP-IDF 6.1 / esp32s3 i walidator N16R8: PASS.
- Pełny zestaw testów hostowych: 503 PASS, bez SKIP.
- Kontrola odsyłaczy/kotwic, przykładów shell/JSON, symboli Kconfig,
  zgodności dokumentacji API oraz `git diff --check`: PASS.

Zmiana dokumentacji; nie zmieniano kodu, konfiguracji ani zależności.
Bez flash i nowych pomiarów sprzętowych. Maksymalny stabilny bitrate
poszczególnych kodeków oraz kontrolowany pomiar przyspieszenia SIMD
pozostają OPEN. Zachowano wcześniejsze lokalne zmiany.
