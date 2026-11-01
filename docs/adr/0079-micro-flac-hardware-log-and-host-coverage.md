# ADR0079 — pierwszy log micro-flac i rozszerzenie prób hostowych

Data:2026-11-01. Status: pomiar S3 użytkownika odebrany; stabilność micro-flac FAIL,
przyczyna błędów OPEN. Bez zmiany zachowania firmware w tym kroku.

## Log sprzętowy5c378192

Firmware dd9e9a0, backend micro-flac0.3.0, TEST FLAC native lossless.
Log obejmuje około242s od zasilenia. Rozdzielamy trzy zjawiska:

1. Gen1 i2: TLS handshake-0x2700 i HTTP_CONNECT przed startem odtwarzania.
   Przypięty mbedTLS definiuje ten kod jako X509_CERT_VERIFY_FAILED.
   Szczegółowa flaga certyfikatu nie jest widoczna w uciętych liniach Reader;
   nie przypisujemy przyczyny automatycznie brakowi SNTP, CA ani serwerowi.
   Trzecia próba łączy się. Nie wyłączamy weryfikacji TLS.
2. Gen3 po około66s dekodowania: vendor-13 = BAD_LPC_PARAMS. Gen4 po
   około129s: vendor-3 = CRC_MISMATCH. W obu momentach Reader ended0,
   errorESP_OK, HTTP200, read1024. To dwa potwierdzone błędy dekodera
   przy nadal aktywnym czytniku; player dopiero wtedy zamyka sesję/retry.
   Nie stanowią dowodu na uprzednie zerwanie Wi-Fi/TCP podczas grania.
3. Niezależnie liczne niedobory: low7/31/7 w sesjach3/4/5, łącznie45
   raportów Underrun; ostatnia sesja trwa nadal na końcu logu. Compressed
   schodzi do1B, cache PCM i pending często0; najczęściej rośnie empty,
   np. gen4 empty951/busy40. Przyczyna dopływu/buforowania nieustalona;
   samo empty nie identyfikuje miejsca opóźnienia w całym torze.

## Wydajność i pamięć

22 próbki profilu po warm-up, średnie arytmetyczne z logu:

| Pomiar | Minimum | Maksimum | Średnia |
|---|---:|---:|---:|
| CPU0 busy |39.2%|46.2%|44.3%|
| CPU1 busy |27.2%|38.7%|35.9%|
| task player, procent jednego rdzenia |22.1%|31.9%|29.3%|

Największy codec_max13098us, block_max28041us. Nie porównywać średnich
kumulacyjnych maksimów jako średniego czasu dekodowania. Nie wyznaczamy
speedupu wobec innego logu z innymi utworami. Przykładowe FFT avg około3ms,
gaps/drop blisko1/2: brak podstaw do przypisania tych przerw przeciążeniu FFT.
Wynik PCM36864B odpowiada4608 ramek stereo32, nie rozrostowi scratch do MiB.
Minimalny raportowany zapas internal38192B, PSRAM free około6.90MB;
brak zgłoszonego błędu alokacji. Te dane nie dowodzą braku wszystkich
opóźnień cache/szyny, ale nie wskazują na stałe nasycenie CPU.

## Dodatkowy eksperyment hostowy

Testy z ADR0078 pracowały na64-bitowym hoście, więc micro-flac wybierał
uint64_t bit_buffer i64-bit CLZ. S3 wybiera uint32_t i32-bit CLZ.
Dodano wariant micro32: kopia przypiętych źródeł wyłącznie w katalogu
tymczasowym testu wymusza zgodnie oba warunki UINTPTR_MAX w publicznym
nagłówku i compiler.h. Oryginalne źródła/hashes w repo nie są zmieniane.
To test32-bitowej logiki bitreadera na64-bitowym ABI; nie emulator Xtensa,
nie test asemblera LPC i nie pomiar wydajności S3. Pełny host build-m32
nie jest dostępny z powodu braku32-bitowych nagłówków C++.

Dodano deterministycznie zmienne porcje1..4096B obok1/7/257/4096/65536B.
Wejście kopiowane do wielokrotnie używanego bufora i zatruwane po każdym
wywołaniu, tak aby stała kopia całego pliku nie maskowała zachowania po
ponownym użyciu pamięci przez sieciowy pump. Testy nadal obejmują CRC,
reset, końcową krótką ramkę i live join z oryginalnym bootstrap.

Oracle FFmpeg: micro64, micro32 i dr dają zgodny PCM. Również178 pełnych
ramek wcześniejszego capture tej samej stacji przechodzi wszystkie sześć
profili porcji oraz reset. Nie odtworzono błędów-13/-3 z nowego utworu;
nie ma podstaw do zmiany kodu vendored ani wyłączenia CRC/ASM.
409 testów hostowych PASS; rozszerzone backendy6 ASan/UBSan PASS (LSan off).
Native IDF6.1 build dr PASS; kod firmware i biblioteki bez zmian.

## Następny krok

Porównanie dr_flac z identycznym transportem, ring/progami i widgetami
według FLAC_BACKENDS_BRINGUP. Zachować bieżący wynik jako negatywny odbiór
stabilności micro-flac, nie jako dowód wadliwego wejścia ani całej biblioteki.
Jeżeli błędy powtórzą się również w dr, potrzebny kontrolowany test lokalnego
pliku lub dokładne bajty problematycznej ramki, aby oddzielić input od
dekodera/architektury. Buforów nie powiększamy równocześnie z tym A/B.
Agent nie wykonywał flash/erase ani zmian NVS/playlisty.
