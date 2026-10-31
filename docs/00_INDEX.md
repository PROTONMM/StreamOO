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

Etap06c: `adr/0020-pcm-fifo-and-pre-gain-tap.md`.

Odbiór niedoboru/przeciążenia PCM: `PCM_STRESS_TEST.md`, fragment `../config/pcm_stress.defaults`.

Etap06d — pierwszy MP3: `adr/0021-mp3-decoder-file-poc.md`, `../assets/audio/README.md`.

Etap06e — AAC-LC ADTS: `adr/0022-aac-lc-adts-file-poc.md`.

Etap06f — incremental decode: `adr/0023-incremental-file-decode.md`.

Etap06g — źródło skompresowane i bufor: `adr/0024-compressed-source-buffer.md`.

Etap06h — niezależny czytnik: `adr/0025-independent-compressed-reader.md`.

Etap09a — SoftAP: `adr/0026-secured-softap-bringup.md`, `NETWORK_BRINGUP.md`.

Etap09b — portal odczytu i OLED AP: `adr/0027-readonly-portal-and-ap-oled.md`.

Polityka AP bez audio: `adr/0028-ap-setup-without-audio.md`.

Przyszłe źródła sterowania RC5/telefon: `adr/0029-future-control-clients.md`.

Kilka sieci i prywatny import playlist: `adr/0030-multiple-wifi-and-private-playlist.md`, `../tools/check_playlist.py`.

Konfiguracja WWW/STA i trwały import CSV: `adr/0031-web-configuration-wifi-and-playlist.md`; odbiór: `NETWORK_BRINGUP.md`.

Otwarty lokalny panel i HOME/IP po STA: `adr/0032-open-local-portal-and-home-ip.md`.

Katalog LittleFS i lista OLED: `adr/0033-stored-catalog-to-oled.md`.

Player HTTP/ICY06i: `adr/0034-http-icy-network-player.md`; pierwszy odsłuch: `HTTP_PLAYER_BRINGUP.md`.

Diagnostyka przyczyn retry06j: `adr/0035-live-decode-failure-diagnostics.md`.

Start STA przed AP i IP/VOL/belka: `adr/0036-sta-first-and-volume-row.md`.

Rdzeń pomiarów stereo07a: `adr/0037-stereo-level-analysis-core.md`.

Pomiary stereo na OLED07b: `adr/0038-live-stereo-meter-task.md`, `DSP_BRINGUP.md`.

Wygładzanie mierników07c: `adr/0039-meter-ballistics-and-gap-hold.md`.

Żywsza reakcja mierników: `adr/0040-more-responsive-meter-ballistics.md`.

Referencyjny rdzeń FFT07d: `adr/0041-spectrum-reference-core.md`.

Widmo z rzeczywistego audio07e: `adr/0042-live-spectrum-bringup.md`.

Korekta DC i opadanie FFT07f: `adr/0043-spectrum-leakage-and-release.md`.

Rzeczywisty kodek i Wi-Fi RSSI w HOME: `adr/0044-live-codec-and-wifi-info.md`.

Sprawdzanie playlisty/kodeków na komputerze: `PLAYLIST_PROBE.md`, `adr/0045-host-playlist-stream-probe.md`.

Odstęp RSSI od wizualizacji: `adr/0046-rssi-visualization-spacing.md`.

Zegar SNTP i strefa przez WWW: `adr/0047-sntp-clock-and-web-timezone.md`, `CLOCK_BRINGUP.md`.

Data dzień–miesiąc–rok i trwałość strefy: `adr/0048-day-first-date-and-timezone-persistence.md`.

Bitrate deklarowany przez serwer ICY: `adr/0049-server-declared-stream-bitrate.md`.
