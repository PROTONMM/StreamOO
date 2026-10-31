# 07 — FFT, Hann, SIMD/DSP, wskaźniki

**USTALONE:** trzy przełączalne widgety audio: FFT, 2-kanałowy bargraph i dwa małe wskaźniki analogowe. Analiza audio przed regulacją głośności, nie w callbacku I²S ani blokującej ścieżce dekodowania.

## DSP
- ESP32-S3 Xtensa LX7 PIE, preferować zoptymalizowane ESP-DSP dla S3 i odpowiednie wyrównanie danych. SIMD fixed-point to możliwość, nie założenie o każdym dekoderze.
- Domyślnie FFT długość N=1024, opcja N=2048, porównanie `int16` kontra `float32` z testami nasycenia/normalizacji. 50% overlap, ~25 fps render (parametry **PROPONOWANE**).
- Symetryczne okno Hanna dokładnie: `w[n] = 0.5 * (1 - cos(2πn/(N−1)))`, n∈[0,N−1]; przeliczone raz i zapisane Q15/float, poprawne skalowanie amplitud.
- Spektrum logarytmiczne, domyślnie 24 pasma mieszczące się w 132px, bez dziur między zakresami; pokrycie pasma zależy od sample rate, brak zakładania tylko 48 kHz.
- Przy spadku CPU pomijaj wizualizacyjne próbki/klatki, nigdy nie blokuj PCM output. Metryki dropped frames i czasów FFT.

## Alternatywne widżety
- Stereo bargraph: RMS + peak L/R, wygładzanie attack/release zależne od dt.
- Dwa analogowe mierniki: wspólne RMS lub kalibrowane VU, osobna rysująca funkcja, niezależna od FFT.
- FFT liczyć tylko gdy jest potrzebna do aktywnego widżetu; RMS/peak dostępne zawsze w lekkiej analizie.

## Akceptacja
Test na syntetycznych sinusach 100Hz/1kHz/8kHz, ciszy, impulsie i sygnale stereo; porównanie z NumPy FFT, wykres okna Hann; amplitudy przy ustawieniach volume 0/50/100 powinny być identyczne dla wizualizacji; pomiar obciążenia i FPS.

## Zaimplementowana warstwa prezentacji (04g)

`ui_widgets` rysuje trzy tryby z gotowego snapshotu poziomów 0..32767.
Nie zależy od DSP i nie liczy FFT, RMS, dB ani smoothingu; producent danych
zostanie podłączony w tym etapie. Syntetyczny `home_probe` aplikacji służy
wyłącznie do sprawdzenia obrazu i nie zalicza żadnych testów akceptacji DSP.

## Podstawa transportu próbek — 06c

Tap przed gain ma niezależny pierścień SPSC i numery bloków ujawniające
odrzucenia. Audio nie czeka na analizę. Obecny odbiorca liczy tylko debug
peak ostatniego bloku; widżety wciąż TEST. Dla tonu peak1036 ma pozostać
stały przy VOL0/50/100. FFT/RMS, kalibracja i prawdziwy model wizualizacji
nie są zaimplementowane. Przyszły DSP powinien rozpoznawać luki przed
składaniem okien FFT. [ADR0020](adr/0020-pcm-fifo-and-pre-gain-tap.md).

## 07a — rdzeń RMS/peak

Własny komponent levels z jawnym API: signed32 stereo48kHz przed gain,
Q15 RMS/peak, domyślnie okna1920 ramek (40ms), uint64 suma kwadratów.
Luki/duplikaty bloków unieważniają stary wynik i resetują niepełne okno.
Dziewięć testów, w tym niezależna referencja sinusów i rzeczywisty PCM tap
przy volume0/50/100. Szczegóły/ograniczenia ADR0037. Brak nowych bibliotek.
Jeszcze bez odbiorcy RTOS i wizualizacji na urządzeniu; istniejący player
i ekran bez zmian. Kolejny krok: niezależne zadanie DSP i snapshot do UI.

## 07b — konsument tap i połączenie z OLED

Metering uruchamia osobny task priority1/4096B, ring4 bloków drop-newest
w internal RAM. Player otwiera przed I2S, zwalnia po I2S-close i ACK taska.
Snapshot generation/timestamp, wygaszenie po200ms, publikacja tylko PLAYING.
Bargraph/analog pokazują rzeczywiste RMS/peak przed gain; FFT pozostaje
„Brak danych”. Obecnie liniowa amplituda Q15 bez smoothing/peak-hold/VU.
Błędy alokacji pomiarów nie przerywają audio. ADR0038; odbiór sprzętowy
według DSP_BRINGUP OPEN. 284 testy i build PASS; bez nowych zależności.

## 07c — spokojniejsze wskaźniki

ADR0039: prezentacja10Hz, RMS attack160ms/release500ms, peak hold400ms
i release800ms. Okna RMS nadal40ms, UI/gesty bez spowalniania. Krótkie
luki tap nie kasują ostatniego kompletnego pomiaru; expiry200ms nadal
obowiązuje od ostatniego kompletnego okna, a STOP/generation czyści od razu.
To skala liniowa Q15, nie docelowa kalibracja dB/VU.

Strojenie07c-2 / ADR0040 po uwadze użytkownika o nadmiernym uśrednianiu:
RMS attack30/release150ms, peak hold150/release300ms. Takt nadal10Hz,
próbki i okna RMS bez zmian; ochrona przed miganiem zachowana.

## 07d — matematyczny rdzeń FFT

Komponent spectrum: własny referencyjny float FFT1024/2048, Hann N-1,
DC mean removal, coherent gain, stereo jako średnia mocy, 24 rozłączne
przybliżone log-pasma. Maksimum amplitudy binu na pasmo, skala liniowa Q15.
Testy z NumPy rFFT, dwie długości/3sample rates, sinusy/impuls/szum,
fazy stereo, Nyquist i rzeczywisty tap volume0/50/100. ADR0041.
Nadal brak live FFT/overlap/SIMD i benchmarku float-vs-sc16; ESP-DSP1.8.2
sprawdzono jako przyszły backend, jeszcze bez nowej zależności. Mierniki
RMS/peak pozostawione bez zmian, zgodnie z prośbą użytkownika.
