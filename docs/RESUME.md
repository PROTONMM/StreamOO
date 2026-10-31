# Punkt wznowienia StreamOO

Aktualizacja: **2026-10-31, 09e — STA przed AP i nowy wiersz IP/VOL gotowe do odbioru**.
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

Teraz odbiór09e według NETWORK_BRINGUP: zapisane STA bez AP, fallback dopiero
po całej liście; IP przyciemnione po lewej oraz jasne VOL/belka. ADR0036.
266 testów i native build PASS (0x15b530, 7% wolne). Bez flash/erase/eFuse.
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
