# 14 — Macierz śledzenia wymagań

| Wymaganie | Dokument | Test |
|---|---|---|
| HW-01 piny menuconfig | 02 | build Kconfig + gpio validator |
| HW-02 SSD1322 grayscale | 02,04 | test_ssd1322 + display_smoke; uruchomienie potwierdzone przez użytkownika, szczegóły pending |
| UI-01 teksty ekran 16/8/8/32 | 04 | golden PNG + OLED |
| UI-02 trzy wymienne wizualizacje | 04,07 | przełączanie bez restartu |
| INPUT-01 gest A/B | 05 | zestaw sekwencji enkodera |
| AUDIO-01 kodeki | 06 | próbki testowe każdego formatu |
| AUDIO-02 volume 0..100 | 06 | kształt gain i brak trzasków |
| AUDIO-03 streaming/rebuffer | 06 | opóźnienia w sieci |
| DSP-01 Hann poprawny | 07 | NumPy referencja |
| DSP-02 SIMD benchmark | 07 | cykle CPU int16 vs float32 |
| ST-01 import yoRadio | 08 | round-trip realnych danych |
| NET-01 AP i STA | 09 | first boot & credentials error |
| NET-02 unique mdns | 09 | dwa urządzenia w LAN |
| WEB-01 nowy REST i auth | 10 | API integration/security |
| NVS-01 5-second debounce | 11 | clock fake + reboot |
| FLASH-01 jedna factory, bez OTA | 11 | tests/test_partition_layout.py; przewodowy flash i ponowny import stacji (ADR0055) |

Canvas/clipping: `tests/test_canvas.py`, zamrożony framebuffer w `tests/fixtures/`, ASan/UBSan `tests/canvas_sanitizer.c`; odbiór nowej sceny OLED pending.
