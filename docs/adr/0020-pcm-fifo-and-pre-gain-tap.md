# ADR 0020 — FIFO PCM, rebuffer i odgałęzienie przed gain

Data: 2026-10-25. Status: przyjęty dla 06c; odbiór sprzętowy OPEN.

## Interfejs i transport

Własny komponent pcm: stałe stereo signed32/48kHz, bloki po240 ramek.
Nie dodano pakietów ani skopiowanego kodu. Używamy C11 stdatomic i istniejącego
audio_gain. Dostępność sprawdzona w kompilacji ESP-IDF6.1 i hosta. Kod własny;
FreeRTOS/esp_driver_i2s pozostają zależnościami adaptera z poprzednich etapów.

Pierścień jest SPSC: dokładnie jeden producent i jeden konsument, z pamięcią
przekazaną przez właściciela. Pojemność potęgi2 w zakresie2..64, init/reset
wyłącznie przy zatrzymanych obu stronach. Liczniki uint32 read/write mają
release/acquire; arytmetyka unsigned i maska działają także przy wrap.
Push na pełnym zwraca false, pop na pustym false; brak nadpisywania danych,
alokacji, mutexów i oczekiwania. Sequence jest metadanymi przekazanymi przez
producenta i odczytywanymi opcjonalnie przez konsumenta.

W profilu GCC ESP-IDF6.1 flaga -mdisable-hardware-atomics wyłącza ogólną
deklarację lock-free rodziny atomików. Pierwszy build ze zbyt szerokim
ATOMIC_INT_LOCK_FREE==2 słusznie został odrzucony. Ta deklaracja obejmuje
też RMW, których nie używamy. Dla wyrównanych 32-bitowych load/store
sprawdzono rzeczywisty assembler: memw/l32i/s32i, bez wywołań __atomic_*.
Zostawiono assert rozmiaru/wyrównania; nie obchodzimy go zmianą typu na
volatile. Przy zmianie toolchainu trzeba powtórzyć audyt kodu wynikowego.

## Aplikacja i położenie FIFO

Źródło sinusoidalne generuje teraz zadanie audio_control, poza callbackiem.
Bufor źródła:32 bloki,7680 ramek/160ms,61440B PCM +128B sekwencji.
Bufor tap:4 bloki/20ms,7680B PCM +16B sekwencji. Duże tablice są w BSS
wewnętrznego RAM (adresy ELF sprawdzone); meta/pipeline w DRAM. Nie jest
to PSRAM ani bezpośrednia pamięć DMA: CPU kopiuje do DMA sterownika.

Obecna kolejność: producent → raw PCM FIFO → tap → gain/rampa → DMA I2S.
To świadomy wariant diagramu docelowego z06_AUDIO: większy FIFO trzyma
surowe próbki, żeby komenda volume nie czekała160ms na już przeliczone dane.
Po gain jest dotychczasowe DMA20ms, bez dodatkowego dużego FIFO. Budowa
pełnego pipeline będzie wymagała dalszego dopasowania do dekodera.

Callback pobiera jeden blok, próbuje skopiować go do tap, nakłada gain.
Wszystko jest ograniczone rozmiarem bloku. Bez generacji tonu, float,
FFT, alokacji, flash, logów i blokującego I/O w callbacku. Kod pcm/gain
w IRAM, dane w RAM; brak wywołań helperów atomowych lub kodu flash w ścieżce.

## Niedobór danych

Tryby pipeline: BUFFERING, ACTIVE, FADING. Startup/restart dopiero przy
>=24 blokach/120ms. Przy spadku do<=10 bloków zaczynamy rampę do zera,
wykorzystując do50ms prawdziwego PCM w rezerwie. Po zakończeniu rampy
BUFFERING podaje zera i czeka na odbudowę zapasu, potem rampuje do
bieżącego żądanego gain. Rozpoczęta rampa do zera kończy się nawet gdy
producent wróci wcześniej. low liczy takie wejścia w ochronne wyciszenie;
starts obejmuje także pierwszy start, nie tylko powroty po niedoborze.

Nieoczekiwany twardy brak bez rezerwy jest liczony osobno: ostatnia surowa
próbka jest podtrzymana tylko na czas rampy do zera. To awaryjne ograniczenie
skoku amplitudy, nie obietnica braku artefaktów DC. Tap dostaje wtedy zera,
nie wymyślone próbki. Normalny SPSC przy stopniowym wyczerpywaniu ma użyć
rezerwy, więc hard powinno pozostać0. STOP przy danych w ACTIVE nadal konsumuje źródło i gainuje
do zera; testowy generator nie jest pauzowanym materiałem sieciowym.

## Tap i diagnostyka

Tap jest przed gain, także przy VOL0 i STOP dostaje rzeczywiste próbki
ciągłego źródła. W BUFFERING dostaje zera. Pełna kolejka tap odrzuca NOWY
blok, zwiększa tap_drop i nie blokuje audio. Sequence odpowiada kolejnym
blokom wyjściowym, rośnie również przy odrzuceniu, co ujawnia luki i wrap.
To numer względny, nie timestamp fizycznego DAC ani generacja stacji.

Osobne niskopriorytetowe zadanie odczytuje tap, liczy jedynie debug peak
ostatniego bloku i luki. Brak FFT/RMS/kalibracji i podłączenia do widżetów.
Dla stabilnego tonu tap_peak16 powinno wynosić1036 niezależnie od volume;
widżety nadal są syntetyczne. Parametry testu: PCM_TAP_DELAY_MS (domyślnie0),
PCM_STALL_TEST (domyślnie n, pauza producenta400ms raz po3s od pierwszego PLAY).
Pauza nie blokuje przyjmowania komend. Stan ochronnego wyciszenia/podbuforowania
UI pokazuje jako BUF zamiast PLAY, gdy żądane odtwarzanie jest włączone.

Niedobór PCM nie jest pełną detekcją DMA underrun/jitter. IRQ opóźnione
przez system mogą spowodować replay bufora bez odpowiedniego sygnału tej
polityki. Potrzebne są pomiary max_fill, obciążenia i ciągłości na sprzęcie.
Brak dekoderów, sieci, adaptive buffer, runtime zmiany formatu i generacji
źródeł/flush przy zmianie stacji. Reset aktywnego pierścienia jest zabroniony;
przyszły player musi skoordynować producenta/konsumenta i stale-response guard.
