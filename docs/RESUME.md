# Punkt wznowienia StreamOO

Aktualizacja: **2026-10-25, AAC incremental decode06f odebrane logiem i odsłuchem**.
PCM/rebuffer/tap stress potwierdzone logiem i odsłuchem (`5664a90`).
Gałąź feature/streamoo-implementation, szczegóły STATUS i ADR0023.

## Checkpoint na koniec dnia — 2026-10-25

Praca zatrzymana na prośbę użytkownika po odbiorze AAC06f.
Kod: `3b18324`; zapis odbioru: `cfd4917`. Gałąź:
`feature/streamoo-implementation`. Brak rozpoczętej implementacji kolejnego etapu.

Przy wznowieniu zacznij od interfejsu źródła skompresowanego i ograniczonego
bufora z backpressure, najpierw z deterministycznym źródłem testowym;
reader sieciowy będzie następnym krokiem. Zachowaj pojedynczych właścicieli,
obsługę końca danych/błędu i brak blokowania/alokacji w callbacku I2S.
Nie powtarzaj odebranych testów AAC06f ani PCM stress bez nowego powodu.

Pozostawiona konfiguracja i obraz: AAC-LC incremental, stress OFF,
tap_delay0; start STOP i volume/widżet z NVS. Płytka przetestowana przez
użytkownika, dźwięk OK. Ostatnia weryfikacja kodu:152 testy hostowe,
ASan/UBSan i build wariantów PASS. Checkpoint zmienia tylko dokumentację;
nie wykonywano ponownie buildu ani flashowania.

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

**Katalog i widżety nadal są syntetyczne. MP3 z pliku jest nowym PoC06d;
nie ma sieci, strumieni, pozostałych kodeków, produkcyjnego playera,
analizy DSP ani WWW.**
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

## Historia06d — MP3 batch PoC (zastąpione w06f)

MP3_FIXTURE_TEST=y zastępuje sinus własnym plikiem4s MP3 L/R/stereo,
przypięty esp_audio_codec2.6.2. Licencja Espressif Modified MIT (ESP-only),
Apache notice MP3 zachowane w licenses. Zarejestrowany tylko MP3.

Decoder działa przed I2S, daje PCM16 do1MiB PSRAM, handle zamykany po
przygotowaniu. Producent/pcm_adapter konwertuje do istniejącego SRAM FIFO
stereo32/48kHz. ISR nie dotyka PSRAM ani dekodera. To batch predecode,
nie streaming ani benchmark czasu rzeczywistego. Brak resamplingu. Start STOP z NVS.
Klip zawiera ciszę, więc peak tap jest teraz zmienny — nie oczekuj1036.
Widżety/katalog nadal TEST. Obecnie źródło wybiera choice w menuconfig: AAC-LC, MP3 lub sinus.

Log użytkownika dla3f761ae: MP3 PoC ESP_OK,64512B→193536 ramek
(dokładnie referencja),48kHz/stereo16, peak985, decode337274us, wall501083us,
stack_hwm22744. PCM low0/hard0/starts1, invalid0, tap_drop/gaps0, max_fill133us.
Stabilny heap, zapis NVS volume83. Jednorazowo nieczytelny fragment konsoli
około16s — użytkownik wyjaśnił, że poruszył przewodem UART i polecił kontynuować.

## Bieżące06f — decode w trakcie odtwarzania

AAC06e odebrane logiem i czystym odsłuchem („tak, wszystko grało ładnie”).
Referencja193536 ramek,48kHz/stereo16. Dotychczasowy runtime MP3 także
potwierdzony logiem. Nie powtarzać testu batch decode ani PCM stress.

Nowy streamoo_decode_stream:257B input +8192B PCM, przechowuje ogony,
wydaje240 ramek na żądanie producenta tylko przy miejscu w SRAM FIFO.
Usunięto alokację1MiB całego PCM w PSRAM. Dekoder żyje w zadaniu audio_control,
EOF resetuje parser/dekoder. ISR/gain/tap bez zmian; NVS i start STOP zachowane.
Źródło nadal testowy plik we flash, nie sieć; HE-AAC/resampling/player/DSP OPEN.

Lokalny profil AAC-LC, stress n/0, encoder A5/B6/BTN7 REVERSED=y.
Odbiór AAC06f zaliczony: użytkownik potwierdził „Dżwięk OK”.
Log3b18324: work_bytes8528, cycles1..5/ok1, frames193536/cykl,
max_read4034us, stack_hwm22728B. PCM queued27..30/ACTIVE, low0/hard0,
starts1, invalid0, tap_drop/gaps0, max_fill130us. Heap internal211223B,
PSRAM8341720B stabilne10–20s. max_read to pull240 w zadaniu, nie ISR.
Nie powtarzać zaliczonego odsłuchu. MP3 incremental i nowe próby VOL/STOP
nie zostały osobno potwierdzone; test długotrwały i sieć nadal OPEN.

152 testy hostowe PASS; ASan/UBSan pump/konwertera PASS (bez LeakSanitizer).
Build/link AAC-LC, MP3 i sinus PASS. Testy hostowe używają sztucznego
backendu; prawdziwy dekoder wymaga S3.
Następny krok: oddzielny interfejs źródła skompresowanego i bufor
z backpressure, potem reader sieciowy. Nie ogłaszać pełnego radia.
Przy pierwszej konfiguracji wymagany dostęp do rejestru Component Manager;
managed_components ignorowane przez Git, dependencies.lock śledzony.

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
