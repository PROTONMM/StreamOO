# 12 — Plan testów i ukończenie projektu

## Bramy jakości
1. **Hardware smoke:** menuconfig + validacja GPIO, SSD1322 init, pełna skala 0..15, aktualizacja ekranu, reakcja enkodera.
2. **UI:** wiersze 16/8/8/32, PL glyphs, scroll, centralna stacja, trzy widżety, płynność UI; porównania framebufferów z obrazami referencyjnymi.
3. **Audio PoC:** streaming MP3/AAC do PCM5102A, poprawny I²S, zero-click volume ramp i izolacja FFT od I²S.
4. **Codec extended:** FLAC, Ogg Vorbis i Opus w realnych strumieniach i kontenerach (każdy osobno potwierdzony); pomiar CPU/PSRAM i underrun.
5. **DSP:** Hann N−1, 1024/2048, int16 vs float32 referencja, test częstotliwości/RMS/peak, 25fps bez dropów audio.
6. **Stations:** test prawdziwego formatu yoRadio, walidacja, merge/replace, atomic commit, eksport round-trip, metadane i trwałe ID.
7. **Network:** first boot AP, stałe hasło testowe, unikalny SSID, błędny credential, AP off po STA, mDNS wiele urządzeń, zmiana Wi-Fi WWW i rollback.
8. **Web:** własny REST, autoryzacja administracji, odporność na błędne payloads, ograniczenia upload.
9. **Reliability:** wielogodzinne odtwarzanie, Wi-Fi flap, TLS failure, zanik flash podczas importu, restart, presja heap.
10. **Delivery:** instrukcja flash, instrukcja użycia, BOM, partition map, dependency/license report, znane ograniczenia, protokół testów.

## Warunek uznania za ukończone
Brak krytycznych otwartych kwestii, poprawny `idf.py build`, testy automatyczne i sprzętowe udokumentowane z wynikami. Sam build nie dowodzi, że firmware działa na sprzęcie.
