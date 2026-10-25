# Plan i indeks dokumentacji

Punkt wznowienia pracy: [RESUME.md](RESUME.md).

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

Implementacyjne ADR: `adr/0001-n16r8-gpio-policy.md`, `adr/0002-platform-and-bringup-gate.md`. Procedura odbioru PCB: `HARDWARE_BRINGUP.md`.

Etap 04a i moduł msalamon: `adr/0003-ssd1322-msalamon-spi-bringup.md`.

Etap 04b i canvas: `adr/0004-independent-4bpp-canvas.md`.

Etap 04c-1 i referencyjny font PL: `adr/0005-font-reference-and-utf8.md`.

Etap 04c-2 i polecenie importu fontu: `adr/0006-provisional-polish-font-import.md`; stan licencji: `../THIRD_PARTY_NOTICES.md`.

Etap 04d: `adr/0007-text-alignment-and-marquee.md`. Metryki commitów: `COMMIT_METRICS.md`.

Etap 04e: `adr/0008-krypton-and-alpha-font-rendering.md`; źródło fontu `../assets/fonts/README.md`.

Etap 04f, scroll tylko w lewo: `adr/0009-left-only-scroll.md`.

Etap 04g, niezależne widżety: `adr/0010-visualization-widgets.md`.

Etap 04h: `adr/0011-ui-owner-and-station-list.md`.

Etap 04i: `adr/0012-home-snapshot-renderer.md`.

Etap 05a: `adr/0013-encoder-decoder-characterization.md`.

Etap 05b: `adr/0014-encoder-irq-diagnostics.md`.

Korekta sprzętu enkodera: `adr/0015-confirmed-encoder-wiring.md`.

Etap 05c: `adr/0016-encoder-ui-command-adapter.md`.

Etap 05d / pierwszy zakres NVS: `adr/0017-deferred-ui-preferences.md`.

Etap 06a: `adr/0018-i2s-output-tone-bringup.md`.

Etap06b: `adr/0019-controlled-audio-gain-ramp.md`.
