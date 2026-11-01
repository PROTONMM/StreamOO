# Punkt wznowienia StreamOO

## Najnowsze — kontrolowany FLAC LAN, bez zmiany firmware / ADR0091

Log45424bc0 potwierdza b2431e5/dr/PS_NONE, lecz50 low w~3min i1 Recover.
CPU średnio40,6/42,1%, HTTP read max1,141s, log95,9ms. Odsłuch OPEN.
Nie stroimy kolejnego parametru na innym utworze live. Gotowe3 pliki300s
i playlist.csv w ignorowanym build/flac-lan-20261101; instrukcja FLAC_LAN_TEST.
FLAC16~213kbps,24bit~1839kbps i ten sam24bit bez nagłówka. Uruchomić
python http.server8000 w tym katalogu i zaimportować CSV przez WWW.
Test po3min każdej stacji z UART; koniec300s/EOF jest oczekiwany.
440 testów hostowych, HTTP/SHA i realny dr/FFmpeg PCM32 dwa pełne cykle/reset PASS.
Firmware na płytce b2431e5 pozostaje; źródło CRC i niedoborów nadal OPEN.

## Najnowsze — test Wi-Fi bez modem sleep / ADR0090

Załącznik3d8f1f16 potwierdza bf40138/dr,97 low,2 Recover, gen3 bez retry.
HTTP read do674ms, log batch do82,8ms, CPU~43%/43%, FFT~2,85ms.
OLED aktualizacje działają: około88% mniej payload. Odsłuch nieoceniony.
W starym logu pm type1: aktywny domyślny modem sleep. Nowy krok ustawia
WIFI_PS_NONE po starcie drivera/przed connect i potwierdza przez get_ps.
Błąd/niezgodność ostrzega, bez zatrzymania sieci/AP. Reszta toru bez zmian,
większy pobór energii Wi-Fi. Do odbioru3–5min tej samej stacji od resetu:
wersja, power save=NONE, Delivery/Underrun/Recover/CPU/OLED UI.
437 testów hostowych i native dr/ESP-IDF6.1 build PASS; agent nie flashował.
Źródło złych bajtów CRC i stabilność nadal OPEN.

## Najnowsze — niedobory FLAC i pomiar HTTP/logów / ADR0089

Załącznik5ad29183: napis „Brak danych”, bez możliwości odsłuchu. dr_flac,
82 low,2 Recover, gen3 bez reconnect; FIFO i wejście okresowo puste.
CPU średnio42,7/43,4%, FFT~2,84ms, gaps/drop1. Nie jest to odbiór audio.
Brak wersji i OLED UI nie potwierdza wgrania6b90399.
Nowy Delivery zastępuje okresowy flow: HTTP body delta/kbps/age, read max,
push wait max i poprzedni max czasu pakietu logów. Underrun z RX age.
Bufory/progi/FFT/I2S bez zmian. Test tego samego dr/FLAC3–5min od resetu,
z wersją, Delivery/Underrun/Recover/CPU/OLED UI; odsłuch gdy dostępny.
434 testy hostowe i native dr/ESP-IDF6.1 PASS; agent nie flashował.
Źródło uszkodzonych bajtów nadal OPEN.

## Najnowsze — OLED rozwiązany, optymalizacja SPI / ADR0088

Użytkownik potwierdził usunięcie problemu ekranu po ostatnim buildzie;
nie podał nowego identyfikatora. W nowym kroku wysyłki zależą od pikseli:
identyczny framebuffer bez SPI, zmieniony tylko zakres wierszy. UI40ms
zachowane dla animacji; dodatkowe8KiB PSRAM, bez alokacji w present.
Pierwszy/wake pełne, nieudany transfer unieważnia cache. Nowy tryb do
odbioru: scroll, lista, STOP/PLAY oraz log OLED UI checked/sent/payload.
Potwierdzenie poprzedniego buildu nie jest odbiorem nowych transferów.
432 testy hostowe, stress ASan/UBSan i native dr build PASS; agent bez flash.

## OLED B3/B1 — poprawa potwierdzona / ADR0087

Inny firmware na tym sprzęcie OK, przygasa tylko tekst w wierszach słupków.
Profil panelu zamiast resetowych timingów: B3 0x50→0x91, B1 0x74→0xE2.
Bez zwiększania prądu/napięć, bez zmiany szachownicy/FPS.9 testów i build dr
PASS; użytkownik potwierdził usunięcie problemu w późniejszej wiadomości.
Przyczyna fizyczna niezmierzona. W tym samym firmware resync FLAC ADR0086.

## Najnowsze — FLAC resync po potwierdzonym CRC / ADR0086

Nowy log ae2a00bd/218ecb2: przerwy słyszalne,72 Underrun i dwa odrzucenia
frame_limit_no_boundary przy128KiB. Źródło błędnych bajtów/CRC OPEN.
Fallback przy limicie/EOS tylko po CRC8+CRC16 następnej dobrej ramki,
log Recover, bez powiększania buforów. Naprawiono cache drain dr na EOS.
427 host tests/ASan/UBSan i native dr PASS; potrzebny odbiór S3.
Komputer:80s capture,956 pełnych ramek, prawdziwy ICY i3 backendy PCM PASS.
Użytkownik: OLED przygasa tylko w tych samych wierszach co bargraph,
na innym firmware OK. Prąd nie jest główną hipotezą; profil timingów ADR0087.

## Najnowsze — wynik dr_flac i dokładna diagnostyka parsera / ADR0085

Log85dc8a81 z firmware73fa94e potwierdza dr_flac-0.13.4 na S3.121 ostrzeżeń
Underrun,3 błędy -1001 przed właściwym dr decode; Reader aktywny/ESP_OK.
CPU0/1 średnio41,36/43,98%, odsłuch niepełny, stabilność FAIL.
Dodano error-only flac_parse Reject: reason/status, bytes/scan/CRC,
BOS/EOS/metadane i maksymalnie16 bajtów prefiksu. Bez zmiany buforów
i dekodera. Kolejny log ze stacji TEST FLAC native lossless ma zawęzić
przyczynę -1001. Sama wymiana biblioteki nie rozwiązała niedoborów.
Z tego logu nie ocenimy szachownicy bargraphu (starszy firmware).
Aktualny serwer sprawdzony z komputera:302→200, obie domeny TLS PASS
z CAfile SDK. HTTPS w firmware startuje przed potwierdzeniem SNTP;
do wyjaśnienia początkowego -0x2700 potrzebny UTC i pełne tls_flags.

## Najnowsze — szachownica bargraphu / ADR0084

Logo wygląda poprawnie, wskazówki nie pulsują (potwierdzenie użytkownika).
Bargraph na jego polecenie: szachownica1×1 GS10/czerń, ciągła ramka GS10,
peak GS15. 423 host tests, ASan/UBSan, build dr PASS. Do wgrania i oceny
wyglądu oraz pulsowania; przyczyna nadal OPEN. Bez zmian spectrum/audio.

## Najnowsze — pulsowanie OLED / ADR0083

Gołym okiem dół pulsuje w rytm słupków; wyżej czasem przygasa tekst.
Brak pustej klatki wysyłanej przed obrazem. Opcjonalny test
STREAMOO_LCD_TIMING_PROBE (domyślnie n): HOLD/REPEAT/LOAD25/LOAD10/MOVE,
5x8 s po PLAYING, audio działa dalej. Enkoder przerywa; potem zwykły UI.
Procedura OLED_TIMING_BRINGUP.md; wyniki obserwacji i przyczyna OPEN.
Nie zmieniono profilu OLED ani audio. Porównanie dr_FLAC nadal osobnym
otwartym odbiorem; do diagnostyki OLED wybrać stabilną stację MP3.

## Najnowsze — poprawiony Python dla build_flac.py / ADR0081

Po rich_click wrapper nie używa już bezwarunkowo sys.executable. Wybiera
IDF_PYTHON_ENV_PATH/bin/python lub canonical venv zgodny z wersją SDK,
ustawia środowisko potomne i nie rozwiązuje symlinku do /usr/bin/python3.
Zmiana interpretera zachowuje stary build jako previous-python-N i tworzy
świeży katalog; sdkconfig.flac-* oraz managed_components pozostają.
418 host tests i native dr build z rodzicem /usr/bin/python3 PASS.

Użytkownik: aktywować SDK przez . "$IDF_PATH/export.sh", następnie
python3 tools/build_flac.py dr flash --port /dev/ttyUSB0. Oczekiwany log
backend=dr_flac-0.13.4; nadal czekamy na właściwy odsłuch dr. Agent nie flashuje.

## Najnowsze — pewny wybór dr i pomyłka poprzedniego polecenia / ADR0080

Logc7f8d36f i ELF2262539b9 w build/flac-dr nadal micro. Samo -B w instrukcji
agenta nie wymuszało właściwego SDKCONFIG po zmianach/czyszczeniu. Stary
build/flac_dr.sdkconfig zniknął; porównanie dr na S3 nadal nieprzeprowadzone.
Nowy tools/build_flac.py micro|dr|esp build|flash --port PORT trzyma
sdkconfig.flac-* poza build/, podaje go jawnie w obu etapach i sprawdza
nagłówek oraz binarkę przed flash. Nazwane katalogi mają też guard CMake.

Następny test: python3 tools/build_flac.py dr flash --port /dev/ttyUSB0
(w aktywnym IDF6.1). Oczekiwany backend=dr_flac-0.13.4. Nie korzystać
z wcześniejszego skrótu samego -B. Bez zmian NVS/playlist i bez erase.
Agent wykonał tylko buildy.414 host tests,3 sprawdzone buildy i negatywny
test CMake PASS. Firmware micro1516.33KiB,dr1529.83KiB,Esp1530.52KiB;
nie oznacza to jeszcze rankingu CPU/RAM runtime. W najnowszym logu micro
tylko5 Underrun do21.4s, potem stabilne low5 do182s, bez błędu decode.

## Najnowsze — micro-flac S3 niestabilny, następny dr / ADR0079 / 2026-11-01

Log5c378192: CPU0~44%,CPU1~36%,player~29%, decode max13.1ms, blok28ms;
micro zwraca-13 BAD_LPC_PARAMS i-3 CRC_MISMATCH przy aktywnym ReaderESP_OK.
45 raportów Underrun, źródło często empty. Pierwsze dwie próby boot TLS
mają-0x2700, potem połączenie działa; nie mieszać ich z późniejszymi decode
fail. Konkretna przyczyna błędów i niedoborów nadal OPEN, micro nieodebrany.

Testy rozszerzone: 32-bitowy bitreader/CLZ na64-bitowym hoście przez kopie
tymczasowe, porcje zmienne, ponownie używane/zatruwane wejście.409 testów,
backend6 ASan/UBSan i178 ramek wcześniejszego capture PASS. Nie jest to
emulacja32-bitowego ABI/ASM. Oryginalne źródła i firmware bez zmian.

Teraz test dr_flac z takim samym transportem/progami/buforami/widgetem.
Przygotowany katalog build/flac-dr (SDKCONFIG build/flac_dr.sdkconfig);
alternatywnie wybór dr w menuconfig. Potwierdzić log backend=dr_flac-0.13.4.
Zebrać CPU/perf/buffers/Underrun i błędy od startu, minimum5min/zmiana utworu.
Agent nie flashował, nie zmieniał NVS ani prywatnych playlist.

## Najnowsze — trzy backendy FLAC / ADR0078 / 2026-11-01

W Kconfig StreamOO/Audio and DSP wybór Espressif/micro-flac/dr_flac;
domyślnie micro-flac. Mikro przyrostowy, dr trwały handle i własny cache
pełnych ramek PSRAM, oba PCM32. Transport/progi/bufory/I2S bez zmian.
Źródła/licencje/hashes przypięte w components/decoder/third_party.
Krótka końcowa ramka7 próbek już obsługiwana przez reader granic.
Log3ee4ad16 wskazuje błąd dekodowania-7 przy ReaderESP_OK, nie dowodzi
pierwotnej utraty Wi-Fi. Odsłuch po nowych backendach nadal OPEN.

409 host tests i ASan/UBSan nowych backendów6/actor11 PASS. Realny capture
178 ramek daje PCM identyczny z FFmpeg w obu alternatywach. Native3 buildy
PASS: build/simd-check=micro0x17b150, build/flac-dr=dr0x17e750,
build/flac-esp=Espressif0x17ea10. micro ASM LPC skalarny, dr C skalarny;
FFT nadal SIMD. Nie mylić pozornych ee.* w tablicach danych z SIMD.

Następny krok: flash micro, ten sam FLAC minimum5min/zmiana utworu,
FFT/bargraph; potem wybór dr i analogiczny test. Zachować logi/ELF.
Najlepszy benchmark CPU: ten sam plik lokalny, nie różne utwory live.
Zbierać CPU interval/task, phase, Decode perf/buffers/flow, Buffer interval,
Underrun i błędy Reader/Stream failed. Protokół FLAC_BACKENDS_BRINGUP.md.
Nie wykonano flash/erase; NVS/LittleFS/prywatne playlisty zachowane.

## Najnowsze — przyrostowy parser FLAC / ADR0077 / 2026-11-01

Poprzedni1MiB scratch też okazał się niewystarczający: log6e9a0e50 ma
needed1050624, empty139/busy2, codec_max264ms, CPU około46%/40%.
Teraz własny parser przez publiczne esp_audio_simple_dec_register:
CRC8/CRC16, pierwsza granica, REPARSE po nadejściu danych. Nadal SDK cache,
decoder i STREAMINFO, use_frame_dec=false. Nie przywrócono raw ADR0061,
nie zwiększono buforów/progów. Przyczyna wszystkich przerw nadal OPEN.

403 host tests, header/parser14 i adapter2 ASan/UBSan PASS;178 pełnych
ramek starego capture zgodnych z FFprobe. Native IDF6.1 build/simd-check
PASS0x17e810; GPIO/diff PASS. Agent nie flashował.

Następny krok: zwykły flash bez erase/NVS/playlist, ten sam TEST FLAC native
lossless5min/zmiana utworu. Oczekiwany log `incremental CRC frame parser`.
Zebrać Decode perf/diag, Underrun, phase/CPU; sprawdzić needed1050624,
pcm_max, codec_max i low/starts. Potem MP3/AAC, powrót FLAC, STOP/PLAY/reset.
Kontrakt binarnego SDK/cache/rejestracji/reset i odsłuch S3 nadal OPEN.

## Najnowsze — FLAC: pojemność i WAIT / ADR0076 / 2026-11-01

Po loguda70514: FFT stabilny3.1ms, CPU ma zapas; FLAC low/starts rosną,
a needed525312 przekracza512KiB i restartuje sesję. Live scratch teraz1MiB
PSRAM, przed I2S, bez realloc. Prebuffer/fade/parser i priorytety bez zmian.
Nowy Underrun przed uzupełnianiem PCM: compressed/cache/pending oraz
wait_empty/wait_busy (osobne nieblokujące liczniki źródła, kumulacyjne).

397 testów hostowych, live/HTTP/actor ASan/UBSan i konfiguracje PASS.
Native IDF6.1 build PASS0x17e4a0 w build/simd-check.

Po flashu FLAC3–5min/zmiana utworu, log Underrun/phase/Decode perf/diag/CPU.
Brak restartu na525312 do odbioru, pozostałe przerwy nadal OPEN; src_wait
nie utożsamiać już wyłącznie z siecią. Bez erase/NVS/zmian playlist.

## Uzupełnienie — audyt SIMD i czas FFT / 2026-11-01

[CODEC_SIMD_AUDIT](CODEC_SIMD_AUDIT.md) dokumentuje częściowy SIMD Opus/SILK
i brak wykrytego SIMD w zbadanych MP3/AAC/HE-AAC/FLAC/Vorbis. Zawiera też
pochodzenie pomiarów i orientacyjny iloraz FFT1.5–1.8×: jedna stara próbka
4.104ms kontra wybrane nowe2.3–2.7ms. Nie podawać tej liczby jako potwierdzonego
speedupu A/B lub średnich. Następny krok nadal odbiór da70514 na S3;
brak nowego firmware w tym uzupełnieniu.

## Najnowsze — harmonogram RMS/FFT / ADR0075 / 2026-11-01

Pomiary6f27c8b: SIMD2–4ms, HLS sporadycznie513.6ms czasu ściennego przy
ciągłym PCM; CPU0~36%,CPU1~59%, metering7.5%. FPU przypinało zadanie do
pierwszego rdzenia użycia. Teraz audio_levels jawnie core0 priority4 ponad
reader3, dekoder nadal core1. Profiler dopasowuje handle+xTaskNumber,
chroni reset runtime po ponownym wykorzystaniu adresu TCB przy zmianie stacji.

396 testów hostowych, metering/profiler ASan/UBSan i konfiguracje PASS;
native IDF6.1 build PASS0x17e2c0 (build/simd-check).
Po build/flash HLS64 HE-AACv2 minimum60s FFT/30s bargraph/powrót,
dalej bezpośredni HE-AAC i FLAC. Oczekuj core0 priority4, affinity0x1,
core_samples=N/0; porównać przyrosty drop/gaps i czasy FFT oraz low/hard/starts.
Poprawa na S3 OPEN; bez zmian I2S, ringów, SIMD i maskowania UI.
Agent nie flashował, nie kasować NVS/LittleFS.

## Najnowsze — FFT SIMD i pomiary / ADR0074 / 2026-11-01

FFT domyślnie używa ESP-DSP1.8.2/sc16 AES3, potwierdzone instrukcje SIMD
w ELF native build IDF6.1 PASS0x17e240. Log: backend + last_fft/avg_fft/max_fft.
Hann N-1, stereo power, DC, pasma, takt10Hz i task affinity zachowane.
Skalowanie blokowe odwracane do dBFS,16B-aligned PSRAM, statyczna tabela4KiB.
392 testy hostowe, sc16/float/NumPy, native sc16 ASan/UBSan i walidatory
PASS; hardware/time/CPU OPEN.

Wgrać normalnie bez erase/NVS; TEST HLS64 HE-AACv2 oraz FLAC po60s FFT,
30s bargraph i powrót FFT. Zbierać CPU/player/Decode perf/meters/FFT/heap.
Przed flashem53e7e2b można zebrać baseline; alternatywnie wyłączyć flagę
StreamOO SIMD i porównać `portable-float-radix2` na bieżącym kodzie.
Agent nie flashował. Build agenta jest w build/simd-check, venv w /tmp;
standardowy lokalny venv6.1 nadal wymaga naprawy przez właściciela środowiska.
Procedura i interpretacja: DSP_BRINGUP. Poniższe wznowienia są historyczne.

## Najnowsze — profil CPU i punkt odniesienia FFT / ADR0073 / 2026-10-31

Firmware ma teraz profiler runtime FreeRTOS: po około22s od startu log
`platform` pokaże `CPU interval`, core0/core1 i do sześciu tasków wraz z
maską affinity. `meters` zawiera dodatkowo `core_samples=0/1` dla
nieprzypiętego `audio_levels`. Nie zmieniono przypisań `http_reader` core0 i
`player` core1 ani ścieżki PCM/I2S. Po zwykłym flashu potrzebne są dwa bloki
CPU + player/meters/FFT dla HLS64 HE-AACv2 oraz FLAC. Nie zmieniaj jeszcze
struktur FFT: obiekt GCC15.2.0/-Os jest skalarny FPU, bez SIMD/ESP-DSP; o
optymalizacji zdecydują wyniki pomiaru.

## Najnowsze — krótkie luki widma HLS / ADR0072 / 2026-10-31

TEST HLS64k HE-AACv2 ma ciągłe PCM (`low=0`, `hard=0`), lecz droppable tap
ma luki, więc FFT wygasał po200ms i HOME migał „Brak danych”. HOME wygasza
ostatnią poprawną klatkę maksymalnie800ms wyłącznie dla tego samego streamu
i widżetu, zamiast ją zamrażać. Liczniki meteringu nadal ujawniają luki, a
STOP/zmiana stacji lub przekroczenie limitu czyści ekran. 382 testy hostowe i walidator GPIO PASS;
native build ESP-IDF6.1 nadal blokuje brak lokalnego virtualenv. Po flashu
sprawdzić ten HLS i inne widżety; dźwięk nie może się zmienić.

## Najnowsze — kontrast widma OLED / ADR0071 / 2026-10-31

Pasmo częstotliwości pozostaje logarytmiczne. Po skali -60..0dBFS ekran
używa gamma1,5: `level * sqrt(level)`, więc ciche pasma zajmują mniej
wysokości, a głośne są wyraźniejsze. Korpus słupka ma jasność8, jego
bieżący szczyt jasność15; nie jest to peak-hold. 381 testów hostowych i
walidator GPIO PASS. Natywny build ESP-IDF6.1 nie został wykonany, bo
lokalny export wskazuje brakujący virtualenv. Bez zmian FFT, envelope,
audio/I2S, NVS lub flash. Po flashu sprawdzić wygląd przy mowie i muzyce
oraz brak wpływu na dźwięk.

## Najnowsze — import playlisty dopisujący stacje / ADR0070 / 2026-10-31

Upload playlist.csv przez WWW atomowo dodaje rekordy do katalogu, nie usuwa
istniejących. Stare stable ID pozostają, nowe wpisy mają nowe ID; duplikaty
URL dozwolone/raportowane. Clear i zapis edytora nadal jawnie czyszczą lub
zastępują katalog. Test magazynu, portal/frontend,381 testów i native build
PASS0x17bf70. Odbiór sprzętowy: mały import, suma/stare stacje oraz reboot;
bez erase/importu poprzedniej listy przez agenta.

## Najnowsze — FLAC: rozdzielenie HTTP i dekodera / ADR0069 / 2026-10-31

Log `TEST FLAC native lossless` łączy krótkie fade/rebuffer z `src_wait`, a
nie z pełnym opróżnieniem PCM (`hard=0`). `http_reader` jest teraz core0 wraz
z Wi-Fi, player/dekoder core1. Brak zmian ringów, I2S, NVS, partycji i
playlist. Odbiór: zwykły flash bez erase/importu, logo task placement core0/1,
FLAC kilka minut i zmiana utworu; porównać `low`/`starts`, hard ma pozostać0.
Użytkownik odebrał TEST FLAC native lossless: czysto bez przerw; log116–151s
low0/hard0/starts1/PCM30..32 mimo src_wait2→6. Nie jest to soak/wszystkie
FLAC. 381 testów, targeted player11+HTTP23 i native build0x17bd40 PASS.

## Najnowsze — logo do startu odtwarzania / ADR0068 / 2026-10-31

Na polecenie użytkownika boot: logo minimum1,5s, potem pozostaje podczas
prób Wi-Fi i startu ostatniej zapisanej stacji. Bez przejściowej strony STA
ani wygaszonego STOP. Po PLAYING przejście HOME, AP od razu pokazuje
konfigurację. RETRY lub30s od pierwszego IPv4 odsłania diagnostykę zamiast
bezterminowego logo. Po IPv4 minimum500ms na synchronizację ownera/katalogu.
Obsłużony gest kończy logo; po PLAY/STOP/reconnect logo nie wraca.

Autoplay domyślnie true. Stary blob ustawieńv1 czytany z autoplay=true,
pozostałe pola i ID zachowane; zapisv2 pozwala nadal wybrać STOP przez WWW.
Migracja bez kasowania NVS i bez zapisu flash na ścieżce UI/audio. Brak
zapamiętanej, istniejącej stacji pozostawia HOME do wyboru, bez losowego fallback.

381 testów hostowych PASS (granice logo/AP/timeout i v1/v2/default settings),
native ESP-IDF6.1 build PASS0x17bc90,90% app wolne; GPIO/diff PASS.
Zwykły flash bez erase/importu. Odbiór sekwencji OLED/audio na S3 OPEN.
Nie wykonywano flash. Stabilność HLS/FLAC nadal osobno OPEN.



## Najnowsze — bitrate HLS / ADR0067 / 2026-10-31

HLS używa bitrate wybranego wariantu: poprawne AVERAGE-BANDWIDTH, inaczej
BANDWIDTH, bit/s→kb/s. Media reload/segmenty nie zerują deklaracji; oddzielny
audio rendition bez własnego bitrate nie dziedziczy obcego. Fallback icy-br
pozostaje możliwy przy braku wartości. Bez wiarygodnej deklaracji nadal --.
To deklaracja nadawcy (peak/średnia), nie pomiar VBR ani sample rate.

380 host tests i HLS/HTTP ASanUBSan (LSan off), native ESP-IDF6.1 build
PASS0x17ba90 (90% app), GPIO/diff PASS. Teraz zwykły flash bez erase/importu:
na HLS128k/64k sprawdzić bitrate OLED także po kilku reload. Odbiór OLED
OPEN. Diagnostyka/LSIDfix/logo/bufory bez zmian; brak nowych zależności,
NVS/partycje i prywatne playlisty nietknięte. HLS/FLAC stabilność nadal
osobno OPEN, ROOT CA/FFT/RMS/WWW odłożone, OTA wyłączone. Agent nie flashował.
Starsze sekcje są historią.


## Najnowsze — nazwa stacji i diagnostyka błędów / ADR0066 / 2026-10-31

User: HLS64k-heaacv2-stereo też przerywa; log bf6f3367 kończy295413B
po low0/starts1 i HTTP200/NOT_SUPPORTED, wzorzec ADR0065. Nie potwierdza
aktualizacji (brak Decode perf); nie deklarować kolejnego błędu dekodera.

Dodano Station changed z ID/nazwąPL i generation; Connect/Stream failed
również z ID/nazwą. No-op grającego ID nie loguje zmiany i nie restartuje.
Boot: firmware/IDF/krótki ELFhash z metadanych oraz diagnostics=2. Reader
na awarii: resource/stage/reason, HTTP/MIME, read/rx, HLS sekwencje/liczniki,
errno/TLS kody i flags. Końcowy snapshot przed EOF i refresh przy awarii
zachowują pierwotną przyczynę, nawet gdy pipeline raportuje source_eof.
HWM mierzony także w HLS. Nazwy/kontrolne znaki bounded/sanitized;
bez nowych logów URL/query/haseł. Długie Reader tylko na awarii po fade,
bez zwiększenia okresowego UART w aktywnej ścieżce. I2S/bufory bez zmian.

377 host tests, HTTP/player ASanUBSan (LSan off), native ESP-IDF6.1 build
PASS0x17b990,90% app wolne, GPIO/diff PASS. Test obejmuje nazwęPL/ESC/no-op,
root HLS error po późnym EOF, TLS/errno przed cleanup i HLS counters.
Teraz zwykły flash bez erase/importu. Z logu potrzebne firmware z boot,
Station changed/Connect oraz Stream failed + Reader + Decode perf/diag dla
tej samej gen. Odbiór logów i stabilności HLS/FLAC S3 nadal OPEN. Queryfix
ADR0065 oraz profilowanieADR0064 pozostają w firmware; logo/ustawienia
bez zmian. ROOT CA/FFT/RMS/WWW odłożone, OTA wyłączone, agent nie flashował.
Starsze sekcje poniżej to historia.


## Najnowsze — HLS128k MP3 / ADR0065 / 2026-10-31

Użytkownik wskazał HLS MP3, nie FLAC: początek phase4/44100/PCM32/low0/
starts1, potem HTTP200/NOT_SUPPORTED po~3 segmentach. Przyczyna odtworzona:
SDK6.1 get_url pomija query; po master znikał lsid media URL, reload bez
niego dostawał nowy MASTER, celowo odrzucany podczas grania. Publiczny
serwer potwierdza MEDIA zlsid i MASTER bez;5 segmentów MP3/44100/stereo
sprawdzonych hostowo. Nie dodawać ich audio do repo.

Poprawka: własny pełny request_url i Location, zachowanie query/escaping,
redirect także query-only przez dotychczasowy resolver. Brak SDKget_url/
set_redirection w readerze. LimityURI/redirect i TLS/no-downgrade zachowane,
około4KiB dodatkowo w PSRAM, bez zmian PCM/decoder/ISR/NVS/partycji/playlist.
Stary reader odtwarza błąd na shimie SDK, nowy przechodzi wielokrotne
reload i podpisane redirecty; test obejmuje dwa ID3 jak w realnym źródle.

374 testy hostowe, HTTP ASanUBSan (LSan off), native ESP-IDF6.1 build
PASS0x17aa40 (90% app), GPIO/diff PASS. Teraz zwykły flash bez erase/FS/
reimport: ten sam HLS przez>=3min, jedna gen/starts1/low0 i brak ponownego
MASTER/NOT_SUPPORTED po~37s. Odbiór poprawki na S3 OPEN. FLAC nadal osobno
niedokończony: czekamy na phase + Decode perf z ADR0064; ten log nie dowodzi
naprawy FLAC. Logo/ustawienia bez zmian, ROOT CA/FFT/RMS/wyglądWWW odłożone,
OTA wyłączone. Agent nie flashował. Starsze sekcje poniżej to historia.


## Bieżący problem — FLAC nadal przerywa / ADR0064 / 2026-10-31

Użytkownik: po ADR0062 nadal źle. Nowy log31..51s: gen2, low5→11/starts5→11,
PCM0 mimo dodatnich snapshotów compressed35..138KiB; brak decoder/OOM w tym
fragmencie. Nie przypisywać zrzutów5s dokładnie do chwili niedoboru. FFT0,
max_fill~90us. Nie oznaczać ADR0062 jako naprawione odsłuchem.

Produkcja: input257B→4096B PSRAM, zachowana obsługa1B/partial. Packet work
max256calls/około64KiB, osobne source_waits/work_yields. Backend parsera,
scratch512KiB/ring/PCM/prebuffer bez zmian. Nowa linia Decode perf co5s i
przy awarii: codec_max/block_max, ring przed/po najwolniejszym decode,
rozmiar PCM, źródłowe WAIT/yield oraz postęp wejścia/wyjścia i refill.
To czas ścienny, nie czystyCPU; kod tylko w tasku, bez zmian I2S callbacku.

372 testy hostowe i live/actor ASanUBSan (LSan off) PASS. Shim125000us
potwierdza wrapper/log/reset; porcje4KiB i rozdzielenie WAIT/yield sprawdzone.
Native ESP-IDF6.1 build PASS0x17aba0 (90% app), GPIO/diff PASS. Następny
krok: zwykły flash bez erase/FS/reimport, ten sam FLAC; po przerwach2–3
kolejne pary phase=.../Decode perf... dla tej samej gen. Odsłuch/stabilność
nowej wersji i dokładna przyczyna wciąż OPEN, nie obiecywać naprawy na bazie
host fakes. Agent nie flashował. Logo/ustawienia/HLS bez zmian, ROOT CA i
strojenie FFT/RMS/WWW odłożone, OTA wyłączone. Starsze sekcje są historią.


## Bieżące — 2026-10-31 / FLAC recovery ADR0062 i logo ADR0063

Użytkownik: po ADR0061 gra gorzej. Log bca642b2: memory/NO_MEM,
compressed1, low/starts około2/s, potem raw adapter INVALID_RESPONSE/vendor0.
ADR0061 odrzucone: surowy backend i CRC scanner wycofane; wraca parser SDK
ze STREAMINFO. Hostowe testy CRC nie potwierdziły kontraktu binarnego SDK.
PCM scratch512KiB PSRAM obejmuje138240B i maksymalny blok524280B;
wewnętrzny ring PCM rezerwowany przed codec_open. FLAC start/refill~500ms
z icy-br, fallback64KiB, clamp32..192KiB w istniejącym ring256KiB; bez
restartu TCP/generation. MP3/AAC próg8KiB bez zmian. Odbiór S3 OPEN.

Logo propozycja: dwa O/fale + StreamOO + linia sygnału, native256×64/4bpp,
assets/branding/startup-256x64.png. Pokazywane1,5s tylko po power/reset,
nie na wybudzenie STOP, reconnect ani zmianę stacji; Wi-Fi łączy równolegle.
Renderer bez heap/drugiego framebuffer; tools/preview_boot_logo.py odtwarza
natywny obraz i powiększenie. Ocena wyglądu/timingu OLED OPEN.

370 testów hostowych PASS, live/actor ASanUBSan (LSan off), renderer guards/
podgląd, GPIO/diff i native ESP-IDF6.1 PASS0x17a8a0,90% app wolne.
Teraz zwykły flash bez erase/FS/reimport; nowy log FLAC live frame: supplied
validated STREAMINFO, bez CRC-checked single frames. Sprawdzić FLAC kilka
minut i zmianę utworu: low/hard/starts, compressed/PCM, heap/min/largest.
Nie obiecywać rozwiązania za wolnej sieci samym rebufferem. Odbiór nowych
ustawień/HLS i RNŚTLS nadal osobno OPEN. ROOT CA/FFT/RMS/wyglądWWW odłożone,
OTA wyłączone. Agent nie flashował. Starsze wpisy poniżej są historią.


## Bieżąca dostawa — ADR0059/0060 / 2026-10-31

Punkty2/3/4/8 wdrożone: ostatnia stacja/start STOP lub autoplay, ID trwałe
przy rename/URL, własna nazwa mDNS WWW/NVS i HLS audio. Długi STOP/OLED-off,
krótki PLAY lub lista, konfigurowalne5s/auto-wybór (1..120s lub powrót),
bez restartu tej samej grającej stacji. Commit obsługi: ee1bdbc.

HLS: nieszyfrowane live/VOD/master, packed AAC/MP3 i MPEG-TS; AES/fMP4/
BYTERANGE/LL-HLS jawnie poza zakresem. Live sequence bez replay, osobny
manifest około84KiB PSRAM, HTTP reader16KiB stack. Nie zmieniono partycji,
NVSoffsetów, prywatnej playlisty ani zasad TLS. Odbiór PCB nowych funkcji
OPEN: PLAYER_SETTINGS_BRINGUP oraz HLS_BRINGUP. Zwykły flash bez erase/FS,
Ctrl+F5. `playlist.hls.csv`6 publicznych wpisów i raport ignorowaneGit;
manifesty/segmenty/ffprobe hostPASS. Generator ma3 oryginalne HLSVOD.

368 testów hostowych PASS, targeted ASan/UBSan (LSan off), Chromium390px,
node syntax, GPIO validator i native ESP-IDF6.1 PASS. Obraz0x17a440,
90% app wolne. Bez flash/erase/eFuse przez agenta. Hardware OLED/NVS/mDNS/
TS/HLS/heap i długi soak nadal OPEN; host fakes tego nie dowodzą. FLAC128KiB
oraz RNŚTLS wciąż do potwierdzenia. ROOT CA, FFT/RMS i wyglądWWW odłożone,
OTA wyłączone. Starsze sekcje poniżej są historią, ta jest nadrzędna.


## Najnowszy krok — 2026-10-31 / ADR0059

Zrobione: długi STOP i OLED-off (bez sleep), krótki PLAY przy STOP/lista
przy PLAY, bezczynność listy5s domyślnie, WWW1..120s/auto-wybór lub powrót.
Ten sam grający ID to no-op również w ownerze player/WWW. NVS: ostatni ID
po5s, start STOP/autoplay oraz własna nazwa mDNS. Edytor i LittleFS v2
zachowują ID przy rename/URL, aktywna sesja nie restartuje. Stary katalog
czytany, CSV import/eksport3pola. AP panel bez nowych kontrolek.

Build ESP-IDF6.1, host controls/store/settings/player/discovery/REST oraz
Chromium390px PASS. STOP/autoplay/brak ID, no-op sesji, rename/URL/odczyt
po restarcie i błąd NVS sprawdzone hostowo; fizyczny odbiór OPEN.
Teraz zwykły flash bez erase/FS, Ctrl+F5 i PLAYER_SETTINGS_BRINGUP.
Punkt8 HLS zrealizowany w ADR0060. FLAC128KiB i RNŚTLS wciąż do
osobnego potwierdzenia użytkownika. ROOT CA, FFT/RMS i wyglądWWW odłożone.
OTA wyłączone; bez flash/erase/eFuse przez agenta. Starsze wpisy poniżej
są historią, bieżące zachowanie określa ta sekcja i ADR0059.

Aktualizacja: **2026-10-31 — AP setup-only, LAN odtwarzacz i edytor playlisty gotowe do odbioru**.
Gałąź feature/streamoo-implementation. ADR0034–0035, STATUS i HTTP_PLAYER_BRINGUP.

## Potwierdzone przez użytkownika

OLED/PL font i scroll w lewo, encoder półkrok A5/B6/BTN7/reverse=y, preferencje
NVS, lokalny PCM/MP3/AAC i stress odsłuchem. Sieć STA/IP/AP-off potwierdzona
logiem, nowy HOME z IP w trzeciej linii potwierdzony, lista z wgranej playlisty
i wybór potwierdzone („Działa fajnie”). Nie diagnozować ponownie znaków UART:
wcześniej potwierdzono poruszanie przewodami. Reboot/live-import katalogu
pozostają bez osobnego wyniku sprzętowego.

## Odbiór 06i

Użytkownik potwierdził dobry odsłuch kilku stacji. Log 740c327: PLAYING44100
oraz48000, PCM32, invalid/low/hard0, starts1 per sesja, max_fill111–112us.
HWM player min8704 B, reader6404 B. Heap internal61139–61599 B, min39844 B,
largest31744 B; PSRAM min8031908 B. Nie wnioskować wielogodzinnej stabilności
ani konkretnego kodeka/protokołu z samego rate.

Pojedyncza nieudana próba gen7: HTTP200/ESP_FAIL/decoder0/vendor0,
format22050/2/16; retry gen8 wrócił do PLAYING44100. Przyczyna nadal OPEN. 06j dodaje stage/cause, previous/format i osobną linię
Decode diag z licznikami/in/used/out/needed. Zbierz obie linie dla tej samej
gen przy następnym retry; nie zgaduj HE-AAC ani braku obsługi22,05kHz.
Osobne MP3/AAC-LC, HTTPS/ICY, STOP/zaniki/rebuffer i dłuższy soak jeszcze OPEN.

## Player 06i — architektura

Owner player priority4: komendy ID/generation i volume/toggle, stan snapshot,
nowa sesja dla wyboru, retry tej samej stacji, bez fallback do innej. Otwiera
HTTP dopiero po STA/AP-off i wyborze użytkownika, nie po samym boot.
STOP/brak sieci/AP-active: fade, zamknięcie I2S, async stop i ACK readera,
dopiero potem zwolnienie/reset; UI nie czeka na blokujący DNS/TCP/TLS.
Najnowszy wybór wygrywa podczas zamykania starej sesji. Reorder zachowuje ID
bez restartu źródła; usunięcie pozycji zatrzymuje. Trwałość wyboru w NVS OPEN.

Własne streaming: HTTP/HTTPS/ICY, compressed256KiB PSRAM, start8KiB, ICY
demux i metadane związane z pozycją audio, bounded queue64 markerów.
Live decoder: MP3/AAC-LC ADTS, mono/stereo16bit8..48kHz →48kHz stereo32,
liniowy task-only resampler z fazą między fragmentami/WAIT, EOF bez pętli.
HE-AAC/AAC+, HLS/M3U/PLS, FLAC/Ogg/Opus oraz docelowy SRC/DSP nadal przyszłe.
Wspólne sprawdzone PCM/gain/I2S (32×240, fade50ms) bez alokacji/I/O/FFT w IRQ.
Stary nieużywany FIFO testu przeniesiony z BSS do internal heap przy starcie
testu, aby profil sieciowy nie rezerwował drugiego około60KiB bufora.

HTTPS: pełny SDK bundle, hostname/cert verification, brak downgrade/insecure.
SNTP pool.ntp.org po STA; NTP/certyfikaty wymagają sprawdzenia na urządzeniu.
TLS alokuje w PSRAM. API/licencje przypiętego IDF6.1 przejrzane; Mbed TLS4.1.0
pod wybraną Apache2, parser2.7.0 MIT, Mozilla CA MPL2 z source PEM dołączonym.
Źródła i licencje: licenses/ESP-HTTP-TLS-NOTICE.md. Nie kopiowano GPL/yoRadio kodu.

248 testów hostowych PASS; nowa diagnostyka live/actor ASan/UBSan PASS (LSan off,
bez TSan). Transport/RTOS/decoder/I2S w testach są atrapami; nie dowodzą realnego
TCP/TLS, wydajności kodeka i DMA/SMP. Native ESP-IDF6.1 build PASS, 7% wolnego
app. Fill/pcm_fill w IRAM i pipeline/ring w DRAM sprawdzone w ELF.
Agent nie wykonywał flash/erase/eFuse ani połączeń do prywatnych stacji z hosta.

## 06j i następny punkt

Kod diagnostyki gotowy: pierwsza przyczyna pumpa zatrzaskuje się do nowej sesji,
WAIT nie jest błędem, format22050 jest obsługiwany. Guardy rozdzielone, bez
naprawy na domysł i bez zmiany formatu/resamplera/ISR. UART po komendzie fade,
100ms liczone od aktualnego czasu. 15 wektorów błędów i formatowanie logów rzeczywistego kodu actor
sprawdzone na hoście. ADR0035. Brak nowych zależności i nowych alokacji.

Użytkownik zaakceptował09e: „jest dobrze”. Nie wymaga powtarzania zwykłego
odbioru IP/VOL. Scenariusze RF i dokładne czasy osobno w NETWORK_BRINGUP.
07b uruchamia levels w osobnym tasku metering przez tap4 bloków.
Rzeczywiste RMS/peak w bargraph/analog, FFT nadal „Brak danych”. ADR0038.
Teraz zwykły flash i odbiór DSP_BRINGUP: ruch mierników także przy VOL0,
STOP/PLAY/szybka zmiana bez starych poziomów, heap/stack/max_fill oraz meters.
Po odbiorze: smoothing/peak-hold/skala dB/VU i FFT/Hann/ESP-DSP.
Nowy log potwierdza również PLAYING22050; timeouty HTTP0 przed dekoderem
nie wyjaśniają dawnego gen7. Przy retry nadal analizuj stage/cause i Decode diag.

## Uzupełnienie odbioru

Zwykły idf.py -p PORT flash; zachować NVS/LittleFS, bez erase i obrazu FS.
Po podstawowym odsłuchu pozostały STOP/PLAY/szybkie zmiany,
potem AAC-LC/HTTPS/ICY i zaniki. Logi player: phase/rate/compressed/PCM,
max_fill/invalid/low/hard/starts/stack_hwm/reader_hwm; platform heap internal
free/min/largest i PSRAM po uruchomieniu dekodera. Podstawowy odsłuch już zaliczony; nie powtarzać go bez potrzeby.
Pozostałe scenariusze oznaczać oddzielnie. Dokładna procedura HTTP_PLAYER_BRINGUP.md.

## Stałe zasady i dalszy zakres

Lokalny WWW AP/LAN bez dodatkowego logowania — jawna decyzja użytkownika
zastępuje wcześniejsze auth. Hasło sieci AP nadal stałe/prywatny build,
SSID/hasło tylko na OLED w AP. Zachować istniejący prywatny plik buildu,
nie pytać ponownie o sekret i nie wypisywać go. Origin/custom header/limity
zostają, hasła STA nie są zwracane. AP bez odtwarzania pozostaje wymaganiem.

Prywatny /playlist.csv ignorowany/nieśledzony, nie kopiować do repo/build/fixture
ani logów. 267 rekordów/11 normalizacji U+200B/3 zachowane duplikaty, oryginał
niezmieniony; ID name/URL/wystąpienie, ograniczenia rename/edit w ADR0033.
Nie zmieniać NVS/PHY/app/storage offsetów; obecny krok nie zmienia partycji.

Dalsze: pomiary/odbiór06i, SRC/kodeki, DSP i rzeczywiste widgety, last ID NVS,
edycja trwałych ID, mDNS, OTA, sterowanie WWW i telefon przez API, RC5 i MicroSD.
RC5/GPIO/mapy i platforma mobilna OPEN; nie wymyślać pinów. Licencja modyfikacji
małego fontu PL nadal OPEN na wcześniejszą jawną decyzję użytkownika.
Nie dodawać AI-Model/AI-Tokens do commitów; metryki wyłącznie ze zweryfikowanych danych.

Weryfikacja07a:275 testów PASS, levels ASan/UBSan PASS (LSan off),
native build PASS (0x15b530, 7% wolne); kod analizatora jeszcze nieuruchamiany.

Weryfikacja07b:284 testy PASS, metering/actor/controls ASan/UBSan PASS
(LSan off). Native ESP-IDF6.1 build PASS (0x15bcd0, 7% wolne).
Metering około10KiB internal +4096B stos/TCB na sesję; brak pamięci wyłącza
metry bez zatrzymania audio. Lifecycle: I2S-close, STOP worker, ACK, free;
wyniki tylko PLAYING z aktualną generation i wiekiem<=200ms.
Do sprawdzenia heap i deadline na sprzęcie; agent nie wykonywał flash.

Najnowszy krok07c (ADR0039): użytkownik potwierdził działanie07b, ale ruch
za szybki i krótkie „Brak danych”. Log wskazuje gaps23/drop28 przy PCM32
i invalid/low/hard0. Wprowadzono10Hz, attack160/release500ms i peak hold400/
release800ms. Podtrzymanie ostatniego kompletnego wyniku po luce do200ms,
bez zmiany zasad STOP/generation. 286 testów i targeted ASan/UBSan PASS.
Teraz zwykły flash i ocena spokojniejszego ruchu/braku migania na OLED.

Native ESP-IDF6.1 build PASS, obraz0x15bf20 (7% wolne);
check_config --platform i diff --check PASS. Bez flash/erase/eFuse.

Najnowsze strojenie07c-2 / ADR0040: użytkownik uznał07c za zbyt mocno
uśrednione. RMS attack30/release150ms, peak hold150/release300ms;10Hz
i podtrzymanie przy krótkiej luce bez zmian. Teraz ocena żywszej reakcji
na OLED po zwykłym flash bez erase/NVS.

Weryfikacja:286 testów PASS, ballistics/metering ASan/UBSan PASS (LSan off).
Native ESP-IDF6.1 build PASS,0x15bf20 (7% wolne).
check_config --platform i diff --check PASS.

Najnowszy krok07d: użytkownik zostawia RMS/peak do późniejszego dopracowania.
Nie zmieniać teraz ich parametrów. Dodany spectrum, własny referencyjny
floatFFT1024/2048/Hann N-1/24 pasma; NumPy oracle oraz realny PCM tap
volume0/50/100. Jeszcze niewywoływany, bez nowego obrazu UI, bez potrzeby
wgrywania firmware. ADR0041 opisuje kontrakt/normalizację i audyt
ESP-DSP1.8.2 Apache2/IDF>=4.2/API. Zależności jeszcze nie dodano.
Następnie: adapter ESP-DSP, kolektor50% overlap z resetem luk, policzyć
FFT tylko dla aktywnego widżetu, publikacja generation/timestamp, pomiary
CPU/pamięci/max_fill na S3. Porównanie float/sc16 nadal OPEN.

Weryfikacja07d:295 testów hostowych PASS (oracle NumPy wykonany, bez skip).
Native C FFT/PCM harness ASan/UBSan PASS, LSan off. Native ESP-IDF6.1 build
PASS, obraz0x15bf20 (7% wolne); spectrum kompilowany, nieużywany kod
usunięty przez linker. check_config --platform i diff --check PASS.
Bez flash/erase/eFuse i zmian partycji.

Najnowszy krok07e (ADR0042): FFT live1024/50% overlap,24 pasma -60..0dBFS
max10Hz. Tylko wybrane FFT/HOME/PLAYING, osobny istniejący task metering.
Workspace około45KiB PSRAM, alloc przy pierwszej aktywacji, free przed ACK
przy końcu sesji. Brak PSRAM nie blokuje audio/RMS; brak FFT na innym
widżecie/liście. Reset okna i natychmiastowe maskowanie po zmianie żądania,
generation/timestamps/expiry200ms. Nie zmieniono strojenia mierników.
Referencyjny backend float, bez deklaracji SIMD; ESP-DSP/sc16 po pomiarach.
303 testy, NumPy, targeted ASan/UBSan i native build PASS (0x15f2d0,6% app).
Teraz zwykły flash bez erase/NVS i DSP_BRINGUP: słupki, VOL0, STOP/zmiany,
czystość dźwięku; logi FFT max_fft/frames/skipped/gaps i meters/heap/max_fill.
Agent nie wykonywał flash/erase/eFuse.

Najnowszy07f / ADR0043: użytkownik zgłosił szerokie i migające FFT przy
mowie; pusty wykres bez „Brak danych”. Test Hann/off-bin ujawnił artefakt
dolnych binów zwykłej średniej. DC teraz ważone Hann (różnice względem
pierwszej próbki dla dokładnego zera DC), NumPy oracle PASS. Sam Hann
był i pozostaje. FFT envelope: atak natychmiast, opadanie pełnej skali320ms,
bez zmiany RMS/peak, taktu10Hz i freshness200ms/resetu sesji.
305 testów, targeted ASan/UBSan i build PASS (0x15f3e0,6% wolne).
Teraz zwykły flash i ocena słupków na mowie/muzyce oraz log FFT max_fft/
gaps i meters drop/PCM/heap. Nie uznawać szerokości na log osi za dowód
przecieku. Czasy S3 nadal OPEN bez nowego logu.

Najnowsze: użytkownik potwierdził07f: dźwięk OK, słupki lepsze, zostawić
do późniejszego strojenia. Nie zmieniać teraz parametrów FFT/RMS.
ADR0044 dodaje MP3/AAC po poprawnym PCM i RSSI/ikonę Wi-Fi w dolnym lewym
panelu. RSSI snapshot ownera co1s, bez I/O w UI. Codec czyści się na STOP/
retry/offline; bitrate nadal --. Geometria bez zmian.
307 testów i targeted ASan/UBSan PASS; native ESP-IDF6.1 build PASS,
0x15f690 (6% wolne). Teraz zwykły flash i odbiór MP3/AAC/RSSI na OLED.
Bez erase/NVS/obrazu FS. Czasy S3/max_fft i długi soak nadal OPEN bez logu.
Zegar/data/strefa, bitrate, backend ESP-DSP/sc16/SIMD i reszta roadmapy
pozostają dalsze; nie zakładać wyboru strefy/DST bez decyzji użytkownika.

Najnowszy krok: narzędzie hostowe tools/probe_playlist.py (ADR0045),
na prośbę użytkownika o test dostępności, codec/profile/bitrate i usunięcie
nieaktywnych. Oryginał nietknięty, nowe playlist.active.csv/report.json,
zachowane duplikaty/order/ovol. CLI wymaga ffprobe (lokalnie6.1.1),
bez importu GPL do firmware.314 testów i incremental build PASS;
realne lokalne HTTP MP3/AAC/404 PASS. Prywatnych267 stacji nie odpytano.
Instrukcja PLAYLIST_PROBE.md. Firmware nie zmieniony, nie trzeba flashować.

Najnowsza drobna poprawka ADR0046: RSSI o6px w lewo i węższa ikona Wi-Fi, odstęp od wizualizacji. Native build PASS, nowy widok do odbioru OLED. Audio/FFT bez zmian.

Najnowsze ADR0047: użytkownik wybrał konfigurację strefy przez WWW
(Konfiguracja), DST automatyczne.8 opcji UTC/europejskich, defaultWarsaw,
NVS string streamoo_clock/zone; zmiana live i po reboot. GET/POST clock
z dotychczasowym Origin/header, bez loginu. Panel ma tabs Status/
Konfiguracja i formularz Data i czas. OLED prawdziwe SNTP → data/godzina,
przed callbackiem kreski, czas systemowy UTC/TLS nietknięty. Zegar działa
bez stacji/STOP, nie resetuje marquee. Nie zmieniono FFT/RMS.
328 testów, targeted ASan/UBSan, Chromium390px i native build PASS
(0x160ff0,6% wolne). Teraz zwykły flash, Ctrl+F5 WWW i CLOCK_BRINGUP.
Brak flash/erase/eFuse przez agenta. Pozostałe strefy/pełna baza IANA,
pomiary dryfu i odbiór realnego NTP/reboot nadal OPEN.

Najnowsze ADR0048: użytkownik potwierdził zegar i zmianę strefy. Zażądał
daty dzień/miesiąc/rok i strefy w NVM. Data teraz DD.MM.YYYY, placeholder
--.--.----. Zapis NVS streamoo_clock/zone był już w ADR0047; bez zmiany
produkcji dodano test zapisu i odczytu w świeżym procesie z plikowym NVS.
Test i ASan/UBSan PASS. Wgrać standardowo bez erase/NVS; sprawdzić
nową datę i strefę po power-cycle. Zegar SNTP po boocie może potrzebować
chwili, ale wybór strefy pozostaje nieulotny.

Weryfikacja:329 testów hostowych PASS, clock-settings ASan/UBSan PASS
(LSan off), native ESP-IDF6.1 build PASS,0x160ff0 (6% wolne),
check_config --platform i diff --check PASS. Bez flash/erase/eFuse.

Najnowsze ADR0049: użytkownik nie blokuje pracy na fizycznym teście strefy,
sprawdzi go w międzyczasie. Nie uznawać tego testu za zaliczony.
Dodano bitrate nominalny z icy-br do dolnego panelu. Brak/błąd nagłówka
daje --, bez estymacji z sample rate/bufora. Reset na redirect/sesję/STOP/
retry/offline; wyświetlenie dopiero przy znanym PCM.332 testy i HTTP/actor
ASan/UBSan PASS. Wgrać standardowo bez erase/NVS, sprawdzić np.128k/192k
na stacjach z nagłówkiem. FFT/RMS/clock settings bez zmian.

Native ESP-IDF6.1 build PASS,0x161080 (6% wolne), GPIO validator
i diff --check PASS. Bez nowych zależności, flash/erase/eFuse i zmian partycji.

Najnowsze ADR0050: użytkownik potwierdza bieżący firmware. Dodano do
Status WWW PLAY/STOP/volume i POST /api/v1/player/control. Wspólny owner
z encoderem, explicit idempotent PLAY/STOP, absolute volume0..100,
PLAY przy retry nie toggle; brak AP/offline i brak zastępczej stacji.
Najpierw wybór stacji enkoderem; wybór WWW to kolejny mały krok.
334 testy/actor ASan/UBSan/Chromium390px/native build PASS,0x161f00,6% app.
Teraz zwykły flash, Ctrl+F5 i WEB_PLAYER_BRINGUP: UI/audio/enkoder/suwak.
Nie powtarzać strojenia FFT/RMS. NVS strefy po fizycznym power-cycle
użytkownik sprawdza osobno; nie ogłaszać tego odbioru jako zaliczonego.

Najnowsze ADR0051: użytkownik wymaga AP tylkoWi-Fi/import/strefa, LAN
domyślnie player + klikane stacje. Zrobione z serwerowym gate nowych
endpointów. Playlista: add/edit/delete/save/export/import/clear. CAS
generation string, snapshoty, pusty katalog trwały; aktywna usunięta
stacja stop bez fallback. Rename/URL zmienia ID v1 i może zatrzymać
stację (UI ostrzega). Prywatny playlist.csv i wyniki hostowe nietknięte.
334 testy, targeted ASan/UBSan, Chromium390px i build PASS
(0x165fa0,5% app wolne). Teraz zwykły flash/Ctrl+F5 i WEB_LIBRARY_BRINGUP.
Bez flash/erase/eFuse przez agenta. Nie uznawać fizycznego NVS power-cycle
strefy za zaliczony — użytkownik sprawdza osobno.

Najnowsze ADR0052: użytkownik odebrał WWW/edytor, wygląd później.
Dodano mDNS LAN `http://streamoo-xxxxxx.local/` (sufiks AP małymi literami),
HTTP80 DNS-SD. Nazwa i ewentualny konflikt w logu discovery. TylkoSTA/AP-off,
owner sieci, free/restart po utracie/odzyskaniu Wi-Fi, retry błędu30s.
Nowy managed espressif/mdns1.14.0 Apache2, dokładny hash dependencies.lock.
Wyłączone browse/CLI/AP/Ethernet, danePSRAM/stosinternal4096 priority1.
Maxinterfaces2, ponieważ upstream nie kompiluje się przy1; bez vendorpatch.
335 testów i discovery ASan/UBSan PASS; native build PASS0x16dd00,
37632B/2% app wolne. Partitions bez zmian. Teraz zwykły flash i
MDNS_BRINGUP: .local, audio, reconnect/reboot, heap. Odbiór multicast na
płytce OPEN. Własna nazwa WWW/NVS następny mały krok, jeszcze OPEN.
Nie stroić FFT/RMS ani wyglądu WWW teraz. NVS timezone power-cycle
użytkownik sprawdza osobno. Bez flash/erase/eFuse przez agenta.

Najnowsze ADR0053: użytkownik zgłosił zbyt ciche VOL0..33. Krzywa
teraz kwadratowa amplitudowo `(VOL/100)^2`, zamiast-60..0dB. VOL33
około-19dB,50-12dB; mute/unity/rampa50ms zachowane. Wspólne WWW/enkoder.
Zapisana liczba VOL bez migracji, więc po flash gra głośniej: rozpocząć
odsłuch od niskiego poziomu.335 testów i native build PASS, rozmiar
0x16dd00,2% app wolne. Teraz odbiór odsłuchowy zakresu10/20/33/50.
Odbiór mDNS z ADR0052 nadal OPEN, nie zakładać go z tego zgłoszenia.
FFT/RMS/wygląd WWW odłożone. Bez flash/erase/eFuse przez agenta.

Potwierdzenie 2026-10-31: użytkownik otworzył stronę pod nazwą mDNS
swojego radia i zgłosił „działa”. Podstawowy dostęp .local/HTTP odebrany
na płytce (ADR0052). Reconnect/konflikty/DNS-SD/heap nadal OPEN. Odsłuch
krzywej głośności ADR0053 pozostaje OPEN; wiadomość dotyczyła adresu WWW.

Odbiór ADR0053 (2026-10-31): użytkownik potwierdził, że nowa
charakterystyka głośności jest dobra. Odsłuch krzywej zakończony
pozytywnie; pozostawiamy `(VOL/100)^2` i rampę50ms.

Najnowsze ADR0054/06k: użytkownik zlecił pozostałe kodeki i M3U/PLS,
ewentualnie HLS. Podłączono HE-AAC/v2 w ADTS, native FLAC, Ogg Vorbis i
Ogg Opus z istniejącego esp_audio_codec2.6.2. API/licencje przejrzane,
retained XiphBSD i notice. LivePCM16/24/32-bit mono/stereo8..48kHz,
scratch64KiB/pump PSRAM, player stack24KiB (vendor zaleca20KiB+caller).
M3U/PLS rozwiązuje reader:16KiB/3 dokumenty/5 redirect, względne URL,
bez HTTPS→HTTP; HLS #EXT-X- jawnie NOT_SUPPORTED, osobny etap OPEN.
Katalog/importCSV/stableID nie zmienione, URL playlisty zachowany.

Build -Os + silent assertions (kontrole pozostają; ELF do backtrace).
Factory powiększona WYŁĄCZNIE do pustej luki:0x10000/0x180000. Storage
nadal0x190000/0x100000, NVS/PHY bez zmian. Standardowy pełny flash z
nową tablicą, bez erase/obrazuFS. Obraz0x176230,40400B/3% wolne.
346 host tests, targeted ASan/UBSan i native build PASS; PCM/gain IRAM.
Generator tools/generate_extended_fixtures.py sprawdzony:7 oryginalnych
plików i M3U/PLS plus playlisttest. Nie importować automatycznie do radia.

Teraz EXTENDED_FORMATS_BRINGUP: najpierw publiczne KissFM HE-AAC/RNŚAAC+,
potem FLAC16/24/Vorbis/Opus i M3U/PLS z serwera na komputerze. Każdy
binarny dekoder wymaga odsłuchu S3 i heap/CPU/HWM; nie ogłaszać odbioru
na podstawie hostfakes. HE-AACv2 osobno niepotwierdzone. Ogg-FLAC/chained,
rawOpus/RTP,>48kHz/multichannel/ramki>64KiB nie deklarowane. FFT/RMS i
wyglądWWW odłożone, krzywaVOL oraz podstawowe mDNS wcześniej odebrane.
Bez flash/erase/eFuse przez agenta. HLS opcjonalny, wymaga osobnego readera
segmentów oraz planu miejsca; jeszcze niezaimplementowany.

Najnowsze ADR0055: użytkownik REZYGNUJE Z OTA, chce jedną dużą aplikację,
zgadza się ponownie zaimportować stacje. Factory0x10000/0xEF0000,
LittleFS1MiB przeniesiony na0xF00000. NVS/PHY pozostają (Wi-Fi/volume/
widget/timezone). Starego katalogu nie migrować. Pełny flash z tablicą,
bez erase-flash. Master/plan zmienione, OTA nie jest już zadaniem.

Marker FS inicjalizacji per adres sfs_00f00000/init, aby zachowane NVS
phase2 starego FS nie blokowało formatowania nowej blank partycji. Nadal
bez autoformatowania nierozpoznanego niepustego FS. Conditional ręczne
czyszczenie tylko nowego storage w LARGE_FLASH_BRINGUP, agent nie wykonał.
347 host tests/store ASan/UBSan/native build PASS. Obraz0x176250,
14 130 608B/90% wolnego slotu. GPIO validator/diff PASS.

Gotowa lista do importu: root playlist.codecs.csv,13 wpisów; details.csv
z kodekami i JSONraport, wszystkie ignorowaneGit. MP3stereo/mono,AAC-LC,
HE-AACv1/v2,Vorbis,Opus,nativeFLAC z RadioMast; KissFM/RNŚ/Radio357;
M3URadioMast i PLSRNŚ. RealneHTTPSpróbki/ffprobe i naszresolver hostPASS,
SomaFM403 odrzucone. Nie zmieniono prywatnego playlist.csv.

Teraz user flash/reimport, odbiór nowychkodeków i heap/stack/underrun S3.
Nie deklarować odbioru na podstawie hostpróbek. OTA zamknięte przez
rezygnację, HLS nadal osobnoOPEN. FFT/RMS/wyglądWWW odłożone.

Najnowsze ADR0056: user zgłosił FLAC/RNŚAAC+ nie gra. Dostarczony log
potwierdzony jako TEST FLAC native lossless: TLSvalid/HTTP200, parser
NOT_SUPPORT/vendor-7 na pierwszych257B. Przyczyna: live stream rozpoczyna
ramką0xFFF8 bez fLaC/STREAMINFO, a SimpleDecoder wymaga fileheader.
Dodano własny bounded bootstrapnagłówka RFC9639:16B/CRC8/codednumber,
STREAMINFO42B tylko z jawnych parametrów, niezmienione ramki. Flacfile
passthrough,1Bfragment/reset/bounds przetestowane; bez I2S/DSPchanges.
355 tests, adapter/header ASanUBsan, niezależny FFmpegPCM oracle także
midstream, native build PASS0x1767c0,90%app wolne. GPIO/diffPASS.

Teraz userzwykłyflash i test tego samego FLAC; log nowy
FLAC live frame: supplied validated STREAMINFO, potem oczekiwanePCM.
Odbiór poprawki S3 OPEN. Brak zmian playlisty/partycji/NVS. RNŚAAC+
pozostaje osobnoOPEN: użytkownik nie podał jeszcze logu tej stacji.
Nie zakładać, że ma ten sam błąd. Stream failed dodaje codec enum
0MP3/1AAC/2FLAC/3Vorbis/4Opus, by logi były jednoznaczne.
Agent nie flashował/erase/eFuse. OTA nadal wyłączone decyzją użytkownika.

Najnowsze ADR0057: log RNŚAAC+ wskazał TLSrootnotfound przedHTTP/decode.
Origin podaje GTSRootR4 cross-signed przez nieobecny staryGlobalSignRootCA,
ale zaufanysamopodpisanyGTSRootR4 już w pinnedFULLbundle. Włączono natywne
CROSS_SIGNED_VERIFY + TRUSTED_CERT_CALLBACK (defaults/bieżącysdkconfig).
Bez dokładaniaCA/deprecatedroots/leaftrust/wyłączania hostname/podpisów
lub HTTPfallback. Audyt wykazał HAVE_TIME_DATE=n (SDKdefault); teraz y,
UTCzSNTP. Przed sync HTTPS może chwilowo retry. Wcześniejsza deklaracja
sprawdzania dat była nieprecyzyjna. SDKniezmieniony; CA source/license bez zmian.

Hostopenssl naDOKŁADNYM SDKCAfile potwierdza chain/hostname, odrzuca
wronghostname/expiredleaf. KolejnehostyRevma teżchainOK.17testHTTPPASS,
nativebuildPASS0x176800,90%wolne,GPIO/diffPASS. Brak nowych unitów
udających test callbackaSDK; handshakeS3 nadalOPEN. Teraz user zwykłyflash
bez erase/reimportu, sprawdzić RNŚAAC+ i poprzedniąFLACfixADR0056.
WeryfikacjaFLACS3 też nadalOPEN. Nie twierdzić żeAACdekoderodebrany:
tenlogdotyczyłwyłącznieTLS. Bez flash/erase/eFuse przez agenta.

Najnowsze ADR0058: FLAC po bootstrap ruszył, potem przerywał. Logparsera
maxframe20480, neededPCM82944>65536; HTTP200/pełnycompressed, nie brak
sieci. Host3MiB/179pakietów:24bitstereo44100,4608próbek/frame,max25839B.
Ustawiono maxframesize bootstrap128KiB (limit obsługi, nie pomiar całego
strumienia), PCM scratch128KiB PSRAM. Nie zostawiamy0, bo SDK wybiera20KiB.
I2S/ring/gain/playlist/partycje bez zmian, bezreallocwdecode.
357testów, header/liveASanUBsan, FFmpegoracle noise24bit>20KiB/frame,
dużyPCM82944 regression i nativebuild PASS0x176810,90%wolne.
Teraz zwykłyflash i ten samFLACkilkaminut/zmianautworu; oczekiwanylog
max_frame131072. OdbiórS3/heapOPEN, brakflash/erase/eFuseprzezagenta.
RNŚTLSfixADR0057 nadal osobno do potwierdzenia. ImportROOTCA przyszłość,
OTA wyłączone. FFT/RMS/wyglądWWW odłożone.
