# ADR0042 — pierwsze widmo z rzeczywistego audio

Data: 2026-10-31. Status: wdrożone do kontrolowanego odbioru sprzętowego07e.

## Zakres

Widżet FFT używa prawdziwych próbek przed gain, po konwersji do48kHz.
Hann N-1, N1024, stereo średnia mocy,24 pasma jak ADR0041. Prezentacja
logarytmiczna -60..0dBFS amplitudy szczytowej tonu, saturacja, bez VU/RMS.
Aktualizacja najwyżej10Hz; nie obiecujemy25fps ani czasów SIMD.
Mierniki RMS/peak i ich parametry pozostawione bez zmian.

Własny kolektor przyjmuje bloki240 ramek,50% overlap. Na granicy pełnego
okna może policzyć FFT albo pominąć transformację, zachowując overlap.
Przy takcie100ms większość okien jest celowo pomijana — licznik skipped
nie oznacza błędu audio. Luka/duplikat sequence resetuje niepełne okno,
rollover jest ciągły. Nie interpolujemy próbek. Ostatni kompletny wynik
może pozostać do200ms, po czym jest nieważny.

## Własność i brak blokowania audio

Obliczenia w istniejącym zadaniu metering priority1, nigdy w ISR/playerze/
dekoderze. UI publikuje preferencję FFT tylko dla wybranego widżetu na HOME.
Player przekazuje ją do metering tylko przy PLAYING, bez AP/retiring.
Po zmianie widżetu, wejściu na listę, STOP lub buffering FFT nie jest
liczone. Powrót resetuje okno; numer rewizji żądania natychmiast maskuje
poprzednie pasma, nawet zanim worker wykona zmianę. Nowa stacja to nowa
sesja/generation, zgodnie z dotychczasowym cyklem życia.

Workspace około45KiB w PSRAM, alokowany przez worker przy pierwszym
wybraniu FFT; zachowany do końca sesji, zwalniany przed ACK worker-stop.
Brak tej pamięci wyłącza tylko FFT; RMS/audio działają dalej. Nowa próba
alokacji przy następnym włączeniu widma. Ring tap pozostaje4 bloków w
internal RAM, drop-newest. ISR i jego ścieżka PCM bez zmian.

RMS i widmo mają niezależne timestamps/ważność — brak nowego okna RMS
nie unieważnia świeżego FFT. Getter playera sprawdza oba wieki niezależnie.
Przerwa we wszystkich pomiarach nie może pozostawić zamrożonych słupków.

## Backend i pomiary

Pierwszy odbiór live używa własnego zweryfikowanego float radix2 z07d.
W gorącej pętli przygotowania wejścia zastosowano float zamiast software
double per próbka; sumowanie DC w int64, konwersja raz na kanał. Testy
NumPy ponownie PASS. Brak deklaracji SIMD. ESP-DSP1.8.2 pozostaje
sprawdzonym kandydatem na optymalizację (ADR0041); nie dodano zależności
ani kodu stron trzecich. Po pomiarach rzeczywistych dopiero wybór adaptera
i porównanie FFT float/sc16. Ten krok umożliwia pomiar na urządzeniu,
nie uznaje wydajności reference za odebraną.

Log FFT: available/valid/frames/skipped/gaps/max_fft. Max_fft to maksymalny
czas ścienny wywołania feed z faktycznym FFT, obejmuje też kolektor i możliwe
wywłaszczenia. Max_work obejmuje całą iterację, również pierwszą alokację/
init. Odsłuch, tap drops, stack/heap i deadline I2S obowiązkowo na S3.

## Weryfikacja

303 testy hostowe PASS, w tym6 kolektora: overlap porównany z osobnymi
pełnymi ramkami FFT, luki, skip bez liczenia, reset/rollover, błędne API,
pełna monotoniczność mapowania dB i punkty odniesienia. Worker testuje
wybór/wyłączenie/re-enable, wygaszanie widma i błąd PSRAM bez utraty RMS.
Actor chroni generacje/STOP i natychmiastowe wyłączenie, UI publikuje
24 prawdziwe pasma zamiast placeholdera. NumPy oracle07d PASS.
Collector/metering/actor/controls ASan/UBSan PASS (LSan off, bez TSan).
Native ESP-IDF6.1 build PASS,0x15f2d0,6% wolnego app. Pcm_fill w IRAM.
check_config --platform i diff --check PASS; bez flash/erase/eFuse/partycji.

Odbiór sprzętowy i czasy backendu OPEN: DSP_BRINGUP.
