# GŁÓWNY PROMPT DLA CODEX — zbuduj StreamOO etapami

Jesteś głównym inżynierem firmware ESP-IDF i systemów audio czasu rzeczywistego. Twoim celem jest zbudowanie kompletnego, testowalnego radia internetowego **StreamOO** na ESP32-S3 N16R8 według dokumentów tego repozytorium. Pracuj **w tym repozytorium**, zachowuj jego strukturę i aktualizuj dokumentację razem z kodem.

## Obowiązkowa kolejność
1. Przeczytaj `README.md`, `AGENTS.md`, wszystkie pliki w `docs/` i `docs/STATUS.md`.
2. Zidentyfikuj i oznacz brakujące decyzje oraz punkty do weryfikacji sprzętowej. Nie uznawaj ich milcząco za pewne.
3. Zrealizuj kolejno `docs/01_SCOPE.md`, `02_HARDWARE.md`, `03_PLATFORM.md`, `04_DISPLAY_UI.md`, `05_ENCODER.md`, `06_AUDIO.md`, `07_DSP.md`, `08_STATIONS.md`, `09_NETWORK.md`, `10_WEB.md`, `11_STORAGE_SECURITY.md`, `12_TESTS_DELIVERY.md`. Przy dużych zadaniach dziel je na mniejsze testowalne PR/commity.
4. W każdym etapie: napisz plan, zaimplementuj minimalną funkcjonalność, dodaj testy, uruchom dostępne kontrole (`idf.py build`, `pytest`, format/lint), zapisz wyniki i ograniczenia.
5. Jeżeli narzędzia lub urządzenie nie są dostępne, **nie twierdź**, że build albo test na sprzęcie się powiódł. Zapisz polecenia do odtworzenia testu.

## Twarde wymagania
- Natywny ESP-IDF, target esp32s3, profil N16R8 16 MB flash + 8 MB PSRAM.
- Domyślne piny: SSD1322 MOSI11 SCLK12 CS9 DC10 RESET=-1; I²S DOUT48 BCLK47 WS21; enkoder A5 B6 BTN7 (połączenie sprzętu potwierdzone przez użytkownika; schemat referencyjny ma B16). GPIO konfigurowalne przez menuconfig; walidacja kolizji i ograniczeń ESP32-S3/PSRAM.
- Jeden enkoder. Domyślny tryb A: obrót=volume, krótki=lista, długi=play/stop, wciśnięcie+obrót=przełączanie wizualizacji. Tryb B zamienia krótki i długi; wybór przez Kconfig. W liście: obrót przewija z animacją, krótki zatwierdza środkową stację, długi anuluje. Podczas listy bieżąca stacja gra nadal.
- SSD1322 256×64, własny renderer 4bpp. Ekran: y0..15 nazwa stacji; y16..23 tytuł; y24..31 opis; y32..63: po lewej Wi-Fi + kodek/bitrate, w środku wymienny widget audio, po prawej kalendarz/zegar. Widżety wymienne, clipping. Używaj Krypton dla tekstów większych i bitmapowej 5×7 z polskimi literami dla linii 8px.
- PCM5102A/I²S. MP3, AAC/HE-AAC, FLAC, Ogg Vorbis, Ogg Opus jako cele wymagające weryfikacji dekoderów/licencji/wydajności. Pipeline sieć→bufor→dekoder→PCM→ (odgałęzienie pomiarów przed volume)→volume ramp→I²S; brak FFT w gorącej ścieżce audio. Wyjście płynne bez trzasków.
- Głośność 0..100, 0 mute, 100 0 dB, wykładnicza amplituda / liniowe dB (propozycja −60..0 dB dla 1..100), zapisy do NVS po 5 s ciszy zmian.
- Widmo: poprawne Hann N−1, preferuj esp-dsp SIMD na ESP32-S3, porównaj fft int16 ze float32; 1024 (opcjonalnie 2048), logarytmiczne pasma, RMS/peak niezależne od głośności. FFT tylko w osobnym zadaniu, droppable sample queue.
- 256 KiB skompresowanego bufora w PSRAM jako punkt wyjścia; adaptive startup/rebuffer w czasie, brak automatycznego restartu TCP przy krótkim underrun; nie deklaruj parametrów bez pomiarów.
- Stacje: import/eksport **dokładnego** pliku yoRadio po zweryfikowaniu prawdziwego formatu na przykładach, model wewnętrzny z trwałym ID, trwałe przechowywanie LittleFS, edycja listy bez zatrzymania grającej stacji, atomowa wymiana. Nie kopiuj API starego systemu.
- Sieć: pierwsze uruchomienie SoftAP z unikalnym SSID z MAC i **tym samym hasłem AP na wszystkich urządzeniach** (wstrzykiwanym bez wpisywania do repo); po udanym Wi-Fi STA AP wyłączyć. Zmiana sieci przez WWW; zabezpieczyć odzyskiwanie po zmianie nieudanej. mDNS streamoo-xxxxxx.local, zmiana nazwy WWW w NVS i DNS-SD.
- Oddzielne stany player, network, UI; jedyny właściciel stanu odtwarzania; kolejka poleceń od web/enkodera; stale response guard (generation id), retry backoff; brak automatycznego wyboru innej stacji przy niedostępności.
- Nowy API REST v1 + nowy frontend WWW. Lokalny panel konfiguracji AP/LAN bez dodatkowego logowania na jawne polecenie użytkownika (ADR0032); polityka przyszłego sterowania/aplikacji/OTA osobno. Import pliku, edycja listy, sterowanie, Wi-Fi, OTA etapami.

## Metoda wyboru bibliotek
Preferuj istniejące, aktualnie utrzymywane komponenty ESP-IDF i ESP-DSP / dekodery Espressif po sprawdzeniu oficjalnej dokumentacji, licencji, wersji i punktu przechwytu PCM. Gdy biblioteka nie obsługuje wymaganego formatu, opisz alternatywy i wykonaj PoC. Nie zakładaj automatycznej akceleracji SIMD w każdym dekoderze.

## Warunki dostawy
Ostateczny projekt musi budować się dla esp32s3 i posiadać testy jednostkowe oraz testy na sprzęcie z protokołem wyników, kompletny Kconfig i walidator, sterownik SSD1322, UI, enkoder, audio, DSP, stacje, Wi-Fi provisioning, mDNS, WWW, OTA, NVS/LittleFS, dokumentację instalacji i diagnostykę. Nie ogłaszaj zakończenia, jeśli którykolwiek krytyczny test nie został wykonany lub opisany jako brakujący.

## Pierwsze polecenie do wykonania
Rozpocznij od etapu 02/03: zweryfikuj bazę projektu ESP-IDF i pełny profil menuconfig, przygotuj walidator GPIO oraz smoke-test SSD1322+enkoder. Odpowiedz raportem: co działa, co wymaga sprzętu, jakie komendy uruchomiono, jakie są dalsze kroki.
