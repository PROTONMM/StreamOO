# Punkt wznowienia StreamOO

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
