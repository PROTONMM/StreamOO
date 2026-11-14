# ADR0089 — niedobory FLAC: pomiar dopływu i opóźnień logowania

Data: 2026-11-01. Status: eksperyment diagnostyczny; przyczyna niedoborów OPEN.

## Wynik testu użytkownika

Załącznik 5ad29183-6665-454d-8a4f-0d3e5a979158, SHA256:
`e4196387bd647bb0465828659700d262a1403413c9cf5853ac585a6c4dbf09c9`.
Log od 3,788 do 324,138 s, TEST FLAC native lossless, dr_flac-0.13.4.
Użytkownik zgłasza „Brak danych”, nie miał możliwości odsłuchu. Nie
przypisujemy więc temu testowi oceny słyszalnych przerw ani ich braku.
Brak nagłówka z wersją i wpisów OLED UI z ADR0088; ten log nie potwierdza
uruchomienia 6b90399. Potwierdzenie poprawy wcześniejszych timingów OLED
pozostaje ważne, osobno od niedoborów audio.

- 82 raporty Underrun w gen3, 83 starts pod koniec. To licznik przejść
  ACTIVE→FADING przy niskim PCM, nie liczba twardych braków próbek.
  hard=0 nie oznacza ciągłego dźwięku: profilaktyczne wyciszenie nadal działa.
- W wielu ostrzeżeniach compressed=1, cache=0 i rosną wait_empty. Jeden
  bajt jest celowo zatrzymany do EOS, więc nie ma wtedy dostępnego wejścia.
- W 176,348 i 186,368 s: phase3/BUFFERING, compressed=1, PCM=0.
  Późniejsze FFT valid=0 pokrywa się z uzupełnianiem audio; samo rysowanie
  napisu nie dowodzi awarii renderera ani przeciążenia FFT.
- Dwa Recover: 52,698 s skipped31124/validated25215 i 295,988 s
  skipped26526/validated21681. Brak Reject i reconnect po starcie gen3.
  Recovery z ADR0086 działa, ale pomija uszkodzony prefiks. Źródło złych
  bajtów nadal nieznane; pełnego odrzuconego bufora z urządzenia brak.
- 31 interwałów CPU: średnio core0=42,7%, core1=43,4%, maksima46,5/48,9%.
  Brak dowodu stałego nasycenia. Ostatnia średnia FFT2,839ms, max6,910ms;
  fft_gaps i tap_drop pozostają1 po włączeniu widma. skipped to także
  celowe ograniczenie obliczeń do10Hz, nie liczba zgubionych klatek OLED.
- Początkowe dwa TLS-0x2700 są osobnym problemem przed PLAYING. Brak
  pełnych flag/czasu nie pozwala ustalić ich przyczyny z tego załącznika.

## Eksperyment

Nie zwiększać kolejnych buforów ani czasu trzymania nieaktualnego FFT bez
pomiaru. Dodać dane czytnika HTTP i czasu logowania, bez zmian PCM/I2S,
dekodera, progów rebuffer, affinity, tempa FFT i ustawień panelu.

`player: Delivery` zastępuje okresową linię `Decode flow` co około5s:

| Pole | Znaczenie |
|---|---|
| interval_ms / rx_delta / body_kbps | rzeczywisty interwał i przyrost bajtów body HTTP; kbps dziesiętne, nie deklaracja kodeka |
| rx_age_ms | czas od ostatniego dodatniego wyniku HTTP read; -1 oznacza brak pomiaru |
| read_max_us | największy ścienny czas HTTP read w tej sesji, także timeout/EOF |
| push_wait_max_us | największy okres ponawiania zapisu do compressed ring po braku miejsca lub zajętym mutexie |
| log_max_us | największy zmierzony czas poprzednich okresowych pakietów logów playera w tej sesji |
| wait_empty / wait_busy | istniejące, rozdzielone liczniki WAIT źródła |

rx_age_ms trafia też do Underrun. Last RX jest czasem odebrania body przez
API, nie pomiarem pakietów NIC. Przerwy mogą obejmować backpressure lub
pracę czytnika; sam duży wiek nie dowodzi winy serwera. Body obejmuje ICY,
manifesty i segmenty HLS. Maxima obejmują sesję; nie są histogramem ani
osobnymi wartościami dla każdego interwału. Bez nowego bufora, alokacji,
flash i logów w I2S; snapshot chroniony istniejącym lockiem.

W starym logu 62 kompletne pakiety po7 linii mają średnio901B, max912B.
Przy lokalnym ustawieniu UART115200/8N1 ich sama transmisja wymaga około
78,2ms, max79,2ms. To dolne oszacowanie transmisji, nie pomiar blokowania
taska. FIFO PCM32×5ms=160ms, fade przy10 blokach daje mniejszy zapas do
reakcji. Inne taski również logują przez UART. Tylko8/82 ostrzeżeń przypada
do250ms po ostatniej linii pakietu, więc nie uznajemy logowania za jedyną
przyczynę. Nowy log_max_us pozwala zmierzyć ten wpływ. Okresowy flow jest
pomijany, aby nie zwiększać liczby linii; pełny flow pozostaje na błędzie.

## Sprawdzenie

Test prawdziwego czytnika z fake HTTP/RTOS/czasem: read250ms, busy write
5ms, EOF nie zmienia last RX, dokładnie te same bajty audio i pozycja ICY.
Timeout przed pierwszym body zachowuje brak last RX. Test prawdziwego
aktora: interwał i delta dają192kbps mimo niezerowego początkowego licznika;
RX age125ms, odczyt/push maxima, symulowane123ms logowania widoczne w
następnym raporcie. Pełne wyniki host/build w STATUS.

Odbiór: ten sam backend i stacja, 3–5min logu od resetu, z wersją firmware,
Delivery, Underrun, Recover/Reject, CPU i OLED UI. Możliwy bez odsłuchu,
ale ocena dźwięku pozostaje wtedy OPEN. Niski dopływ z pustym wejściem,
zajętość mutexa, backpressure pełnego bufora oraz długie logowanie należy
rozpatrywać oddzielnie. Jeśli powracają Recover, potrzebny capture z S3
lub kontrolowane źródło LAN, aby zlokalizować uszkodzenie bajtów.
