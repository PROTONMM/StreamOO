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
| 11 | `11_STORAGE_SECURITY.md` | NVS, PSRAM, flash bez OTA, ochrona danych |
| 12 | `12_TESTS_DELIVERY.md` | testy i kryteria końcowe |
| — | `13_DECISIONS_OPEN.md` | jawne decyzje/ryzyka |
| — | `14_TRACEABILITY.md` | macierz wymaganie→test |
| — | `STATUS.md` | postęp zadań |

Materiały dowodowe: `references/YR.kicad_sch`, `references/myoptions.h`, `references/SOURCE_NOTES.md`. Materiał źródłowy nie jest częścią docelowego firmware.

Implementacyjne ADR: `adr/0001-n16r8-gpio-policy.md`, `adr/0002-platform-and-bringup-gate.md`. Procedura odbioru PCB: `HARDWARE_BRINGUP.md`.

Etap 04a i moduł msalamon: `adr/0003-ssd1322-msalamon-spi-bringup.md`.

Pulsowanie/prążki OLED, FPS UI a skanowanie panelu:
`OLED_TIMING_BRINGUP.md`, `adr/0082-oled-pulsation-investigation.md`.
Kontrolowany test transferów i obciążenia OLED:
`adr/0083-oled-controlled-load-and-transfer-probe.md`.
Szachownica i ciągły obrys bargraphu: `adr/0084-checkerboard-bargraph.md`.
Przygasanie w tych samych wierszach: timingi panelu B3/B1:
`adr/0087-ssd1322-panel-scan-and-phase-timing.md`.
Transfer OLED po zmianie pikseli, tylko zakres zmienionych wierszy:
`adr/0088-display-updates-from-frame-differences.md`.
Niedobory FLAC, dopływ HTTP i opóźnienia logowania:
`adr/0089-flac-delivery-and-log-latency-measurement.md`.
Wynik pomiarów HTTP/OLED i Wi-Fi bez modem sleep dla radia:
`adr/0090-wifi-awake-for-live-audio.md`.
Kontrolowane długie próbki FLAC w LAN, także bez nagłówka pliku:
`FLAC_LAN_TEST.md`, `adr/0091-controlled-lan-flac-fixtures.md`.
Wyniki LAN i ograniczenie okresowych logów playera do jednej linii na iterację:
`adr/0092-periodic-player-log-staging.md`.
Bitrate mierzony czasem dekodowanego audio bez nagłówka serwera:
`adr/0093-decoded-audio-bitrate-estimate.md`.
TLS po I2S i test programowego AES/SHA: `adr/0094-software-tls-crypto-profile.md`.
Niedobory publicznego FLAC i kontrolowany profil okna TCP/mailbox:
`adr/0095-tcp-receive-window-profile.md`.
Obejście do testu sprzętowego AES/SHA: inicjalizacja crypto przed I2S:
`adr/0096-early-hardware-crypto-preparation.md`.
Zwykły profil dr/hardware/TCP23040, bramka UTC dla HTTPS i szybki start SNTP:
`adr/0097-default-radio-profile-and-https-clock-gate.md`.
Audyt RAM/stosów i oszczędność przez wyłączenie nieużywanej diagnostyki:
`MEMORY_AUDIT.md`, `adr/0098-radio-memory-audit-and-unused-fixtures.md`.

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

Kontrast widma OLED07g: `adr/0071-spectrum-display-contrast.md`.

Mostek krótkich luk widma07h: `adr/0072-spectrum-short-gap-display-hold.md`.

Profil CPU i bazowy FFT bez SIMD07i: `adr/0073-cpu-profile-and-scalar-fft-baseline.md`.

Wektorowy FFT sc16 ESP-DSP07j: `adr/0074-vector-sc16-fft-and-measurement.md`.

Terminowość RMS/FFT i tożsamość tasków07k: `adr/0075-metering-scheduling-and-task-identity.md`.

Audyt SIMD w kodekach i orientacyjne porównanie czasu FFT:
`CODEC_SIMD_AUDIT.md`.

FLAC: żądanie525312B i rozróżnienie WAIT06p:
`adr/0076-flac-output-capacity-and-wait-diagnostics.md`.

FLAC: kolejny limit1050624B i przyrostowy parser06q:
`adr/0077-incremental-flac-parser.md`.

FLAC: wybór Espressif/micro-flac/dr_flac i pomiary06r:
`adr/0078-selectable-flac-backends.md`, `FLAC_BACKENDS_BRINGUP.md`.

FLAC: pierwszy negatywny odbiór micro na S3 i rozszerzenie prób hostowych:
`adr/0079-micro-flac-hardware-log-and-host-coverage.md`.

FLAC: pewny wybór SDKCONFIG i kontrola binarki przed flash:
`adr/0080-verified-flac-build-selection.md`, `../tools/build_flac.py`.

Python SDK w wrapperze niezależnie od Pythona rodzica:
`adr/0081-idf-python-for-flac-builds.md`.

FLAC: pierwszy potwierdzony wynik dr na S3 i diagnostyka parsera:
`adr/0085-dr-flac-hardware-log-and-parser-diagnostics.md`.
FLAC: odzyskanie synchronizacji dopiero po zweryfikowaniu kolejnej ramki:
`adr/0086-flac-crc-verified-resynchronization.md`.

Rzeczywisty kodek i Wi-Fi RSSI w HOME: `adr/0044-live-codec-and-wifi-info.md`.

Sprawdzanie playlisty/kodeków na komputerze: `PLAYLIST_PROBE.md`, `adr/0045-host-playlist-stream-probe.md`.

Odstęp RSSI od wizualizacji: `adr/0046-rssi-visualization-spacing.md`.

Zegar SNTP i strefa przez WWW: `adr/0047-sntp-clock-and-web-timezone.md`, `CLOCK_BRINGUP.md`.

Data dzień–miesiąc–rok i trwałość strefy: `adr/0048-day-first-date-and-timezone-persistence.md`.

Bitrate deklarowany przez serwer ICY: `adr/0049-server-declared-stream-bitrate.md`.

Sterowanie WWW playerem: `adr/0050-web-player-controls.md`, `WEB_PLAYER_BRINGUP.md`.

Tryby AP/LAN i edytor playlisty: `adr/0051-ap-setup-lan-player-and-playlist-editor.md`, `WEB_LIBRARY_BRINGUP.md`.

Nazwa odtwarzacza w LAN: `adr/0052-lan-mdns-advertisement.md`, `MDNS_BRINGUP.md`.

Krzywa głośności: `adr/0053-more-usable-volume-curve.md`.

Rozszerzone kodeki i M3U/PLS: `adr/0054-extended-codecs-and-playlist-resolution.md`, `EXTENDED_FORMATS_BRINGUP.md`.

Duża factory, bez OTA: `adr/0055-single-large-app-without-ota.md`, `LARGE_FLASH_BRINGUP.md`.

Live FLAC bez nagłówka pliku: `adr/0056-headerless-live-flac.md`.

TLS cross-signed RNŚ: `adr/0057-cross-signed-tls-chains.md`.

Większe ramki live FLAC: `adr/0058-live-flac-parser-window.md`.

Obsługa radia, ustawienia i trwałe ID: `adr/0059-player-gestures-persistent-settings-and-station-ids.md`, `PLAYER_SETTINGS_BRINGUP.md`.

HLS audio: `adr/0060-hls-audio-segments.md`, `HLS_BRINGUP.md`.

ADR0061 odrzucone po odsłuchu S3: `adr/0061-single-frame-live-flac.md`.
FLAC parser/PCM/rebuffer: `adr/0062-flac-parser-recovery-and-buffer-reserve.md`.
Logo OLED: `adr/0063-native-oled-boot-mark.md`, `../assets/branding/README.md`.

FLAC nadal przerywa — porcje wejścia i pomiar terminów: `adr/0064-live-decode-batches-and-deadline-measurements.md`.

Zachowanie sesji HLS przy reload: `adr/0065-hls-session-query-preservation.md`.

Nazwa stacji i źródłowe błędy czytnika: `adr/0066-station-and-reader-failure-diagnostics.md`.

Deklarowany bitrate HLS: `adr/0067-hls-declared-bitrate.md`.

FLAC: dopływ HTTP i dekodowanie na oddzielnych rdzeniach: `adr/0069-flac-reader-decoder-core-placement.md`.

Import playlisty dopisujący stacje: `adr/0070-append-playlist-import.md`.

Logo podczas Wi-Fi i autoplay: `adr/0068-boot-logo-through-autoplay.md`.

Przegląd kodu, normalny cichy profil i testy sprzętowe opt-in:
`RELEASE_REVIEW.md`, `adr/0099-release-diagnostics-tests-and-license-review.md`.
Możliwość nadania MIT, ograniczenia zależności i prawa do fontu PL:
`LICENSING_REVIEW.md`.

Błąd alokacji ciągłego PCM po HTTP200 i rezerwacja przed readerem/TLS:
`adr/0100-reserve-contiguous-pcm-before-http.md`.
