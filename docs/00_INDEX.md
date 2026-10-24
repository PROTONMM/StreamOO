# Plan i indeks dokumentacji

Statusy: **USTALONE** = użytkownik podjął decyzję; **PROPONOWANE** = sugestia techniczna do przetestowania; **OTWARTE** = brak potwierdzenia. Etapy projektowe opisują wymagania i kryteria odbioru, nie stwierdzają ukończenia kodu.

| Numer | Plik | Zakres |
|---|---|---|
| 01 | `01_SCOPE.md` | zakres, wymagania, granice migracji |
| 02 | `02_HARDWARE.md` | PCB, GPIO, menuconfig, walidacja |
| 03 | `03_PLATFORM.md` | ESP-IDF, komponenty, pamięć, RTOS |
| 04 | `04_DISPLAY_UI.md` | SSD1322, fonty, widgety, animacje |
| 05 | `05_ENCODER.md` | jeden enkoder, gesty, tryby A/B |
| 06 | `06_AUDIO.md` | strumienie, kodeki, I²S, buforowanie, player |
| 07 | `07_DSP.md` | FFT/Hann/SIMD, bargraph, wskazówki |
| 08 | `08_STATIONS.md` | lista, import yoRadio, ICY, LittleFS |
| 09 | `09_NETWORK.md` | Wi-Fi provisioning, AP, mDNS |
| 10 | `10_WEB.md` | własne API i panel WWW |
| 11 | `11_STORAGE_SECURITY.md` | NVS, PSRAM, OTA, ochrona danych |
| 12 | `12_TESTS_DELIVERY.md` | testy i kryteria końcowe |
| — | `13_DECISIONS_OPEN.md` | jawne decyzje/ryzyka |
| — | `14_TRACEABILITY.md` | macierz wymaganie→test |
| — | `STATUS.md` | postęp zadań |

Materiały dowodowe: `references/YR.kicad_sch`, `references/myoptions.h`, `references/SOURCE_NOTES.md`. Materiał źródłowy nie jest częścią docelowego firmware.
