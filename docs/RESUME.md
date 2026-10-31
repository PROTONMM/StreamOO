# Punkt wznowienia StreamOO

Aktualizacja: **2026-10-31, SoftAP09a przygotowany do prywatnego buildu i odbioru**.
PCM/rebuffer/tap stress potwierdzone logiem i odsłuchem (`5664a90`).
Gałąź feature/streamoo-implementation, szczegóły STATUS i ADR0026.

## Bieżący09a — zabezpieczony SoftAP

AAC06h odebrane (kod6031941, odbiór ed32109); nie powtarzać. Nowy komponent
network uruchamia AP WPA2 StreamOO-XXXXXX z MAC, DHCP192.168.4.1/24,
max4 klientów. Oddzielny task3/6144B, kod SDKESP-IDF6.1. Brak portalu/STA/
mDNS/HTTP. Prywatny wspólny klucz potrzebny lokalnie, nie w czacie/repo.

Przed flash użytkownik uruchamia `python3 tools/configure_network.py`
i potem `idf.py reconfigure build` w aktywnym SDK. JSON poza repo,
0600; domyślnie ~/.config/streamoo/network.json (XDG_CONFIG_HOME obsługiwany).
Bez niego build działa, ale AP test inactive/ESP_ERR_NOT_FOUND: Wi-Fi off.
Końcowy lokalny obraz celowo bez losowego klucza testowego. Nie wgrywać go
z oczekiwaniem AP; najpierw prywatny build. Narzędzie nie loguje haseł,
nie przyjmuje ich w argumentach, nie wpisuje do sdkconfig/defaults.

Nowa factory1500KiB przez SDK SINGLE_APP_LARGE. NVS/PHY i offset factory
bez zmian. Pierwszy flash09a musi objąć tablicę (`idf.py -p PORT flash`),
bez erase-flash, nie sam app-flash. Instrukcja NETWORK_BRINGUP.md, ADR0026.
179 testów hostowych PASS; natywny build AAC+AP0x10da90B, MP3+AP0x1145d0B
i brak sekretu AAC0x812b0B PASS, factory1500KiB. Klucz testowy usunięty,
cache prywatnej ścieżki pusty, header AVAILABLE0. Brak testowego klucza
w logach i końcowym obrazie potwierdzony.

Odbiór S3 OPEN: AP ready, telefon/komputer z DHCP192.168.4.x i clients>=1,
złe hasło odrzucone, po rozłączeniu clients0. Współbieżnie AAC PLAY/VOL/STOP,
czysty odsłuch i low0/hard0/invalid0, stabilne heap/stosy. HOME wciąż syntetyczne
w zakresie sieci; nie oczekiwać strony WWW. Nie kasować NVS ani flashować
za użytkownika. Następny krok po odbiorze: portal/STA z rollback, potem
reader HTTP/HTTPS. Wartość hasła i admin auth D18 nadal prywatne/OPEN.

## Odebrane06h — niezależne zadanie czytnika

AAC06g odebrane (kod c451643, log3c3ee3f, odsłuch/sterowanie fa20f2c).
Nie powtarzać tego odbioru. Nowy fixture_reader priorytet3/stos4096B posiada
offset pliku i chunk997B. Audio_control nadal właścicielem dekodera i PCM.
streamoo_buffered_source chroni byte_ring mutexem, token generacji odrzuca
stare write/finish po rewind. Nie kopiować pod portMUX/wyłączonym IRQ.
Read lock0→WAIT, rewind/stop/stats timeout5ms w tickach, minimum1tick.

Release: stop + notify + ack do1s, potem destroy/free. Timeout zachowuje
żywe bufory do restartu; bez wymuszonego usuwania readera. Sieć, anulowanie
blokującego I/O, reconnect/player/HE-AAC/resampling nadal OPEN. Szczegóły ADR0025.

Lokalny profil AAC-LC, stress OFF/tap_delay0. Runtime6031941 odebrany:
reader generation1..7/input37039, min stos1980B; cycles1..7/frames193536,
generation2..8/stats_ok1/stale0, waits1..7 po rewind bez niedoborów.
max_read3854us, min stos audio_control22664B, PCM queued28..29/ACTIVE,
low0/hard0/starts1, invalid0, tap_drop/gaps0, max_fill131us.
Heap internal204743B i PSRAM8337620B stabilne w próbkach10–20s.
Gesty/zapisy volume80/87 działają, użytkownik: „Dźwięk czysty, działa jak
poprzednio”. Odbiór AAC06h zaliczony, nie powtarzać bez nowego powodu.

168 testów hostowych PASS, ASan/UBSan adaptera PASS; bez TSan/LeakSanitizer.
Build AAC0x81230B, MP3 i sinus PASS; kod/obraz bez zmian po odbiorze.
Fault injection stop/join, wymuszony stale publish i długotrwały test na S3
nadal OPEN. MP3 z nowym readerem nieodebrane. WAIT na EOF nie jest testem
niedoboru sieci. Następny krok: konfiguracja Wi-Fi zgodnie z09, potem reader
HTTP/HTTPS. Nie dodawać haseł do repo i nie ogłaszać gotowego radia.

## Odebrane06g — źródło skompresowane

Checkpoint8df80f5 odczytany. AAC06f pozostaje odebrane (kod3b18324,
odbiór cfd4917); nie powtarzać tych zaliczonych testów bez nowego powodu.

06g oddziela pump od pliku: interfejs DATA/WAIT/END/ERROR i pierścień
4096B w PSRAM, testowy feeder porcjami do997B. Pełny bufor zatrzymuje zapis
bez utraty bajtów. Wszystkie operacje źródła należą do audio_control;
przyszłego readera z innego zadania nie podłączać bez synchronizacji.
WAIT zachowuje fragmenty wejścia/PCM oraz częściowo złożony blok240 ramek,
nie modyfikuje output i nie publikuje niepełnego bloku do FIFO. EOS na
ostatnim bajcie, reset/rewind w EOF nadal tylko polityką pliku testowego.
ISR/gain/tap, piny i NVS bez zmian. Sieć nadal OPEN. Szczegóły ADR0024.

Bieżący lokalny obraz: AAC-LC, stress OFF/tap_delay0. Log c451643 odebrany:
cycles1..10/ok1, frames193536, compressed3839B, backpressure160→1662,
waits0, max_read3829us, stack_hwm min22728B. PCM queued28/ACTIVE,
low0/hard0/starts1, invalid0, tap_drop/gaps0, max_fill133us.
Heap internal209143B i PSRAM8337620B stabilne10–40s. Gesty i zapis NVS
volume78 bez zatrzymania buforów. Nie powtarzać zaliczonych pomiarów.
Użytkownik potwierdził czysty dźwięk i poprawne VOL/PLAY/STOP („tak”).
Odbiór06g zaliczony; nie powtarzać. WAIT na S3 niewymuszony (waits0),
ciągłość po WAIT sprawdzona dotąd na hoście. Nie deklarować testu sieci.
161 testów hostowych, ASan/UBSan i build AAC-LC/MP3/sinus PASS.
Końcowy obraz AAC0x80d00B; kod bez zmian po logu.
Adapter niezależnego readera implementuje06h powyżej; następnie integracja
sieci zgodnie z etapem09.
Nie mylić testowego4KiB z produkcyjnym punktem wyjścia256KiB.

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

## Odebrane06f — decode w trakcie odtwarzania

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
Etap06g implementuje interfejs źródła i bufor, opis bieżącego odbioru powyżej.
Nie ogłaszać pełnego radia.
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
