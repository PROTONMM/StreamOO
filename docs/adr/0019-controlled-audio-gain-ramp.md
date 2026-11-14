# ADR 0019 — sterowany test audio i gain z rampą

Data: 2026-10-25. Status: przyjęty dla 06b; odbiór sprzętowy OPEN.

Po odebranym I2S 06a łączymy enkoder z rzeczywistym gain i PLAY/STOP na
źródle testowym. To nadal nie jest player sieciowy. Własny audio_gain nie
używa nowych bibliotek ani skopiowanego kodu. Actor aplikacyjny używa
istniejącego FreeRTOS z ESP-IDF 6.1 i odebranego audio_output; API kolejki,
callbacku DMA i wymagania IRAM zachowane z poprzednich etapów.

## Obliczenia próbek

Gain 0..100: 0 oznacza dokładne zero, 100 unity. Dla 1..100 tablica Q16
jest zaokrągleniem 65536 * 10^((60*(v/100-1))/20). Dokładna krzywa i wybrana
rampa 50ms są nadal profilem proponowanym do strojenia na sprzęcie.
Oryginalna tablica powstaje offline; brak libm/float w wydawaniu próbek.
Przy 48kHz rampa ma 2400 ramek, wspólny gain dla L/R. Stały krok i reszta
(Bresenham) zapewniają osiągnięcie celu dokładnie w ostatniej ramce.
Retarget zaczyna się od aktualnej wartości, identyczny cel nie resetuje rampy.
Mnożenie int32 PCM przez Q16 odbywa się w int64, dzielenie ze znakiem
zaokrągla do zera. Przy gain<=unity zachowujemy zakres, w tym INT32_MIN;
nie ma UB ujemnego signed shift. Blok ograniczony do 240 ramek.

Generator ciągły używa dotychczasowej oryginalnej tablicy sinusa 1kHz,
stereo, peak1036 w ekwiwalencie int16 (~-30dBFS PRZED gain). Przy VOL100
maksymalny poziom jest taki sam jak w odebranym teście 06a. Przy VOL50
dodajemy około30dB tłumienia, więc test jest bardzo cichy. Widżety pozostają
syntetyczne, jawnie oznaczone; nie pokazują analizy rzeczywistego PCM.

Callback i funkcje generatora/rampy są w IRAM, dane w DRAM. W ELF/objdump
sprawdzono brak wywołań helperów z flash w obliczeniach gain. Callback ma
wyłącznie pobranie uint32 target pod krótkim portMUX oraz generację/gain.
Nie ma alokacji, flash, logów, blokującego I/O, float ani FFT w tej ścieżce.
Wykonywanie komend i publikowanie target jest poza ISR.

## Właściciel i UI

Nowe zadanie audio_control posiada model volume/running/revision i kolejkę
16 komend. UI kopiuje względne VolumeDelta/Toggle bez oczekiwania; pełna
kolejka odmawia, bez optymistycznego zmieniania wskazań. Czysty model
waliduje komendy, clampuje volume 0..100 i oblicza gain. Audio ISR ma wyłączną
własność aktualnego gain/rampy i fazy, a zadanie publikuje tylko target.
UI odczytuje spójny snapshot modelu, nie mutuje właściciela.

Nowy STREAMOO_AUDIO_CONTROL_TEST domyślnie y wymaga interaktywnego UI,
wybiera IRAM-safe i ma pierwszeństwo nad skończonym I2S smoke. Startuje
w STOP z odczytanym volume. To decyzja dla diagnostyki, nie rozstrzygnięcie
D12 autoplay radia. STOP rampuje do zera i podaje ciszę przy działającym I2S;
PLAY rampuje do gain(volume). Nie ma time-shift. Faza testowego oscylatora
biegnie niezależnie, nie wznawiamy pauzowanego materiału. UI PLAY/STOP
odzwierciedla zaakceptowany cel, nie potwierdzenie zakończenia rampy/DMA.

Katalog pozostaje testowy: browse/cancel nie sterują audio, confirm zmienia
wyłącznie testową nazwę stacji, ton pozostaje ten sam. encoder_demo może
być nadal użyty bez zewnętrznego właściciela w starym trybie/testach hostowych.
Wyłączenie AUDIO_CONTROL_TEST przy włączonym I2S_SMOKE_TEST przywraca
skończoną sekwencję 06a.

## Preferencje i granice

Tylko UI pozostaje producentem aktualizacji preferences. Stan audio ma
volume_revision, dzięki czemu UI rozpoznaje także sklejone A→B→A. Nowe
submit_changed resetuje termin zapisu przy zaobserwowanej zmianie nawet
jeśli końcowa wartość jest identyczna. Widget nadal zgłaszany po każdym
geście. Odczyt snapshotu może opóźnić start debounce do następnej klatki;
nie przyspiesza nowego terminu. Zapis już rozpoczęty może skończyć się
starszym stanem, a nowszy przejdzie następny debounce (jak ADR0017).
PLAY/STOP nie jest zapisywany, kontrolowany shutdown/flush nadal OPEN.

Brak PCM FIFO, dekoderów, sieci, DSP i pełnej maszyny stanów playera.
Brak pomiaru latencji komend/DMA, jitter, underrun i słyszalnych trzasków.
Zadanie co5s loguje statystyki callbacku; błąd bufora, fill>=5ms lub brak
postępu kończy test błędem. Nie stanowi to kompletnej detekcji dropout.
