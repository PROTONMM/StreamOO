# Punkt wznowienia StreamOO

Aktualizacja: **2026-10-25, PCM/rebuffer/tap potwierdzone logiem i odsłuchem**.
Odebrany06b: kod `a62eeb6`, potwierdzenie „Działa, ładnie”, odbiór `eaad2e9`.
Gałąź feature/streamoo-implementation; szczegóły w STATUS i ADR0020.

## Odebrane na płytce

- OLED: HOME/lista, polskie znaki, scroll wyłącznie w lewo, trzy widżety.
  Wygląd czcionki do dopracowania później.
- Enkoder półkrokowy **A5/B6/BTN7**, REVERSED=y: prawo zwiększa. B6 to
  potwierdzony sprzęt użytkownika, wyjątek od B16 w schemacie referencyjnym.
- Gesty trybu A i interaktywny UI. NVS volume/widżetu po5s i restart.
- Audio06a: skończony test L1k/R2k/oba1k na I2S DOUT48/BCLK47/WS21,
  Philips stereo32/48kHz, bez MCLK. Kod edb7c63, odbiór80e146c.

## Nowe w06b — odebrane funkcjonalnie

Nowy domyślny STREAMOO_AUDIO_CONTROL_TEST zastępuje skończony test
ciągłym tonem1kHz stereo. Start w **STOP**, volume wraca z NVS.
Długi klik (tryb A) przełącza rzeczywiste PLAY/STOP, obrót zmienia gain,
VOL0 to cisza. Rampa50ms, gain100 unity. Źródło ma peak~-30dBFS przed
gain; przy VOL50 dochodzi około30dB tłumienia — do odsłuchu ustaw80–100.
Wyłącz AUDIO_CONTROL_TEST, aby wrócić do starego testu06a.

Osobny właściciel przyjmuje VolumeDelta/Toggle przez kolejkę16, UI czyta
snapshot. Czysty audio_gain używa Q16 i rampy2400 ramek bez float. Callbacki
i gain są w IRAM, stan/tablice w DRAM; brak helperów flash w obliczeniach.
UI jest jedynym producentem preferencji; revision zachowuje reset debounce
przy sklejonych zmianach. ADR0019 opisuje architekturę i granice.

**Katalog i widżety nadal są syntetyczne. Nie ma sieci, strumieni,
dekoderów, produkcyjnego playera, analizy DSP ani WWW.**
Potwierdzenie testowej stacji zmienia nazwę, ton pozostaje1kHz.
STOP pozostawia I2S z zerami, nie jest pauzą strumienia. D12 autoplay
radia nie rozstrzygnięte; kontrolowany shutdown/flush NVS nadal OPEN.

## Nowe06c — PCM FIFO/tap, praca podstawowa potwierdzona

Producent tonu działa w zadaniu. Raw PCM FIFO ma32 bloki/160ms, callback
pobiera blok, kopiuje do niezależnego tap przed gain i nakłada rampę.
Tap4 bloki/20ms, odrzuca nowe przy pełnej kolejce i numeruje bloki,
żeby analiza wykryła luki. Wewnętrzny RAM, SPSC acquire/release, bez
alokacji/blokowania/FFT/flash w próbkach. Właściwy DSP nadal niegotowy;
odbiorca liczy tylko debug peak (dla tonu1036 niezależnie od VOL).

Przy rezerwie10 bloków pipeline schodzi rampą50ms do ciszy, BUFFERING
czeka na24 bloki/120ms i wraca płynnie. BUF w UI podczas tej ochrony.
Opcje testowe domyślnie n/0: PCM_STALL_TEST i PCM_TAP_DELAY_MS.

Log użytkownika dla368c724: max_fill129us, invalid0, queued28/ACTIVE,
low0/hard0/starts1, tap_drop0/tap_gaps0, peak1036; stabilnie także po zapisie
NVS volume38. To nie jest test VOL0/50/100 ani wymuszonych awarii.

## Wynik testu stress i aktualna konfiguracja

Log użytkownika dla3659b69: stall16.202–16.602s (400ms), potem do70.882s
mode ACTIVE/queued28, low1/hard0/starts2 bez kolejnych zdarzeń. max_fill113us,
invalid0; tap_drop/tap_gaps rosną przy tap-delay100ms, tap_peak16=1036.
Przyrost1000 buforów/5s zachowany. Test liczników i odsłuch zaliczone: użytkownik potwierdził jedną łagodną
przerwę i potem czysty ton („Tak, grał czysto”).

Lokalny sdkconfig przywrócono do **PCM_STALL_TEST=n / PCM_TAP_DELAY_MS=0**
i przebudowano zwykły profil. Płytka użytkownika nadal może mieć firmware
stress, dopóki sam nie wgra normalnego. Opcjonalny fragment stresu oraz
instrukcja pozostają w config/pcm_stress.defaults i PCM_STRESS_TEST.md.
NVS pozostaje bez zmian; nie włączać stresu przypadkowo przy dekoderach.

## Następny krok

1. Nie powtarzać odebranego PCM stress: log i odsłuch zostały potwierdzone.
2. Wybór/PoC dekodera i adapter PCM z audytem API/licencji oraz
   kompatybilności ESP-IDF6.1. Nie deklarować gotowego streamingu/DSP.

Ostatnia weryfikacja: **132 testy PASS**, ASan/UBSan 11 testów PCM PASS,
w tym stress20k bloków, ESP-IDF6.1 build PASS. LeakSanitizer wyłączony
pod ptrace; bez testu wycieków. ThreadSanitizer nie był uruchamiany.
Wyrównane atomic load/store są natywne z barierami, bez __atomic helperów;
nie przywracać ATOMIC_INT_LOCK_FREE==2 bez analizy flag toolchainu
(-mdisable-hardware-atomics). Kod IRAM/dane RAM sprawdzone w ELF/objdump.
Jitter/DMA replay, latencja, słyszalne przejścia i obciążenie wymagają sprzętu.

```sh
python3 -m unittest discover -s tests -q
source /home/mariuszk/.espressif/tools/activate_idf_v6.1.sh
export PATH=/usr/bin:$PATH
idf.py build
```

Przed zmianami czytaj AGENTS, CODEX_MASTER_PROMPT, indeks, właściwy etap
i STATUS. Nie powtarzaj odebranych kroków. Małe commity + test/build/ADR.
Bez Arduino i kopiowania API yoRadio. Nie kasuj NVS ani nie flashuj płytki
za użytkownika. Bez AI-Model/AI-Tokens i wymyślonych metryk w commitach.
Nie wykonuj alokacji, flash, FFT, logowania ani blokującego I/O w próbkach.

Pozostają otwarte: finalna czcionka/licencja zmian PL, wrap listy, format
czasu/strefa, kroki/ząbek i graniczna szybkość wejść, shutdown/flush NVS,
zanik podczas zapisu, dokładny model/zwory DAC i pełne pomiary audio.
