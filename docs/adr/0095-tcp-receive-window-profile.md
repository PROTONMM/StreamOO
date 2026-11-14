# ADR0095 — kontrolowany profil okna odbiorczego TCP

Data: 2026-11-01. Status: build/host PASS; wpływ na S3 OPEN.

## Wynik poprzedniego wariantu

Załącznik b129d3af-b0d1-4bf6-bc69-dda4d3c177ab, firmware b53faff,
ELF7bac5bd6, SHA256 logu
`a0c21bdf8fb2c6d91da5c3a06fe16e5af4f2422b25c20699205ff9004f446985`.
Użytkownik potwierdza bardzo częste słyszalne przerwy na publicznym
TEST FLAC native lossless. Startup potwierdza software AES/SHA i PS_NONE.

Po połączeniu gen3 (9,427s) do końca logu (71,447s):42 ostrzeżenia
Underrun,30 przy compressed=1, czyli bez czytelnego wejścia poza bajtem
rezerwowanym do EOS. Końcowe low42/hard0/starts43, wait_empty632/busy8.
HTTP body:12 interwałów, średnio1155kbps, maksimum1192kbps. CPU pięć
interwałów: średnio58,04%/38,18%, maksima58,8%/39,3%; http_reader~32,4%
jednego rdzenia. To nie dowód pełnego wykorzystania CPU; dopływ i dekoder
muszą być dalej rozdzielane pomiarem. read_max378004us,
log_step_max12626us, codec_max47708us, block_max53642us.

Recover w32,507s: skipped7429/validated_frame22311/prefix_crc16=9a9c.
Nie ma Reject ani reconnect po gen3. Odzyskanie ramek pomija audio;
nie wyjaśniamy wszystkich przerw pojedynczym Recover. Źródło CRC OPEN,
także przy software AES/SHA. Początkowe TLS -0x2700 wystąpiło dwa razy:
przy UTC4/time_synced0 i UTC1793531251/time_synced1, flags0.
Brak PSA-149 w tym krótkim teście nie zalicza sekwencji LAN→TLS po I2S
z ADR0094, której użytkownik tutaj nie powtórzył.

## Hipoteza i zmiana

Lokalny SDK6.1: MSS1440, domyślne TCP receive window5760 (4MSS), mailbox6;
TLS input16384. Małe okno może ograniczać przepustowość przez WAN mimo
dużego bufora aplikacji. Potwierdzają taką możliwość Kconfig lwIP i
[dokumentacja wydajności Wi-Fi ESP32-S3](https://docs.espressif.com/projects/esp-idf/en/release-v6.1/esp32s3/api-guides/wifi-driver/wifi-performance-and-power-save.html).
To hipoteza dla tego urządzenia, nie ustalona przyczyna.

Wrapper przyjmuje `--tcp-window 23040`:16MSS,22,5KiB. Dobiera mailbox
jako max(6,ceil(window/MSS)+2), tutaj18. Weryfikuje wygenerowany sdkconfig.h
przed flash; zła wartość albo za mała kolejka przerywa operację. Dopuszcza
diagnostyczny zakres5760..32768B, MSS536..1460 i kolejkę do64. Bez flagi
zachowuje istniejące ustawienia wybranego prywatnego sdkconfig.flac-*.
Root sdkconfig i pozostałe backendy pozostają bez zmian. To ustawienie
domyślne dla nowych socketów TCP, także WWW, nie tylko jednej stacji.

Nie zmieniamy ring256KiB, PCM32 bloków/160ms, preroll500ms, scratch,
priorytetów, DSP ani I2S. Software AES/SHA pozostaje dla porównania.
Nie zmieniamy walidacji TLS. Dane TCP potrzebują pamięci wewnętrznej:
potencjalny przyrost samego payloadu to17280B na pełne okno połączenia,
dochodzi narzut pbuf/mailbox. To nie pomiar faktycznej alokacji. Poprzedni
log miał minimum internal40380B; obserwować minimum i largest, także WWW.
Nie włączamy przenoszenia całego Wi-Fi/lwIP do PSRAM.

Kconfig wspomina TCP_WINDOW jako opcję socketu, lecz lokalne sockets.h/c
SDK6.1 nie udostępniają tej opcji. Dlatego profil kompilacji, bez pozornego
setsockopt, modyfikacji globalnego SDK czy nowej zależności.

Startup wypisuje TCP RX window/mailbox/MSS. Istniejąca linia Decode buffers
dostaje `audio_kbps`: estymator compressed/czas PCM z ADR0093, niezależny
od preferowanej na OLED deklaracji icy-br. Porównać z Delivery body_kbps
podczas opróżniania wejścia. Okna czasowe są różne, body zawiera metadane;
przy pełnym ring niższy transfer może oznaczać zwykłe backpressure.

## Próba pomocnicza na komputerze

Dwa równoległe, około10s odczyty tego samego publicznego endpointu HTTPS,
bez zapisu audio, z systemowym TLS/CA i Icy-MetaData0. Żądane window5760
vs23040 dało odpowiednio2346 i3078kbps. Linux zmienił rzeczywiste clamp
na7380 i27900; RTT22,2 i23,8ms, maksymalne odstępy read131,8/353,5ms.
Osobne sesje live, burst startowy i różne fragmenty audio, komputer zamiast
Wi-Fi S3, inny TLS. To wskazówka kierunku, nie kontrolowany benchmark
ESP32, gwarancja wzrostu o procent ani pomiar bitrate kodeka.

## Walidacja i następny odbiór

Testy wrappera: dobór mailbox, granice, idempotencja, zachowanie ustawień,
sprawdzenie nagłówka, blokada flash po rozbieżności. Test prawdziwego aktora
sprawdza audio_kbps także przy innym deklarowanym icy-br. Atrapa logów
testu AP/startup konsumuje argumenty i sprawdza formaty kompilatorem;
wcześniejsze makra ukrywały brakujące definicje testowe i generowały unused
dla zmiennych profilu TLS. Produkcyjny startup przechodzi native build.

454 testy hostowe PASS. Native ESP-IDF6.1 dr build PASS, binarka1566176B.
Wygenerowana konfiguracja dr:
WND23040/mailbox18/MSS1440, AES/SHA software; build wrappera PASS.
Agent nie flashował urządzenia; odbiór stabilności OPEN.

```sh
python3 tools/build_flac.py dr flash --port /dev/ttyUSB0 --software-crypto --tcp-window 23040
```

Sprawdzić startup `TCP RX: window=23040 bytes mailbox=18 MSS=1440`.
Publiczny TEST FLAC native lossless przez minimum3min z odsłuchem i UART.
Zachować Delivery, Decode buffers/audio_kbps, Underrun, Recover/Reject,
CPU i heap. Porównać tempo przyrostu low/starts, nie same liczniki różnych
długości sesji i utworów. Jeśli brak poprawy, hipotezy TCP nie uznawać za
potwierdzoną. Osobno powtórzyć LAN24/headerless oraz LAN→TLS/HLS po I2S.
Rollback eksperymentu: ten sam wrapper z `--tcp-window 5760` przywraca
mailbox6 dla MSS1440; nie wymaga kasowania NVS ani playlisty.

## Wynik na S3: hardware/early init + TCP23040

Załącznik0dcbb56c-2b34-4e29-b36c-0ffd02db6b51, SHA256
`2f7e1f1b3f16a760f658a1363bac2cd3312213bf7d00d4e1bc6f29b72c9473d8`.
Firmware dcd5279/ELF742f9352: hardware AES/SHA, early probe PASS z ADR0096,
TCP23040/mailbox18/MSS1440. Jedna stacja TEST FLAC native lossless,
dr_flac0.13.4; brak zmiany dekodera/ring/PCM/preroll względem e76bbcb5.

Gen1..3 kończą TLS -0x2700/code9984/flags0 przy UTC3/5/7 i synced0.
Gen4 łączy12,929s, STREAMINFO15,439s;143 snapshoty PLAYING15,969..728,169s
(712,200s,11min52,2s). Każdy ma low0/hard0/starts1 i invalid0. Zero
ostrzeżeń Underrun, Recover, Reject i błędów sesji po udanym gen4;
wait_empty pozostaje0. Nie ma wyraźnego dowodu badanej poprzednio korupcji
w tym logu, ale brak Recover nie ustala przyczyny wcześniejszych CRC.

Buffer interval: pierwszy zawiera PCM_min0 podczas startu; po nim PCM_min
co najmniej22/32 bloków (110ms), compressed_min co najmniej48794B.
compressed_max osiąga262144B w każdym interwale, ostatnie min249081B.
Delivery body274..3213kbps, audio_kbps do1788, pod koniec1631. Przepływ
body jest ograniczany przez pełny ring: średnia ani chwilowa wartość body
nie mierzy wtedy maksymalnej przepustowości sieci. push_wait_max2,618375s
i wait_busy577 przy pełnym wejściu nie oznaczają braku danych od serwera.
HTTP read max1,163848s; bufor wystarczył do braku rebuffer w tym fragmencie.

CPU71 interwałów: średnio38,38%/44,85%, maksima47,6%/53,9%. http_reader
w70 raportach średnio13,17%/max19,1% jednego rdzenia, player38,69%/max44,9%.
Fill max91us, codec_max41290us/block_max50406us. To nie pełne wykorzystanie
CPU; podkreśla znaczenie dopływu/buforów w poprzednich niedoborach.

RAM internal podczas grania23423..51891B, historyczne minimum11344B,
najmniejszy raportowany largest12800B. Ostatni heap wolne48339B,
minimum11344B/largest14336B. PSRAM minimum6732956B (~6,42MiB), largest
6684672B. Nie sumować wolnego PSRAM z internal jako zamiennego budżetu
TCP. Wzrost okna poprawił liczniki, lecz zapas internal jest już mały.
Pozostawiamy23040B; nie eskalujemy bezpośrednio do32/64KiB. Większy
profil wymaga osobnego pomiaru alokacji/PSRAM i ruchu WWW/TLS/HLS.

W porównaniu z e76bbcb5 (hardware/early init/TCP5760,17/3/7 niedoborów
w krótkich sesjach) wynik zdecydowanie wspiera hipotezę zbyt małego okna
dla tej stacji/łącza. Live zawiera inne utwory, nie ma kontroli RTT ani
naprzemiennego A/B na tym samym pliku, dlatego nie podajemy procentowego
wzrostu przepustowości ani gwarancji dla wszystkich serwerów.

Użytkownik wcześniej wykluczył teraz odsłuch: zaliczamy tylko obserwowane
liczniki. Odsłuch, dłuższy soak, równoczesne WWW, zmiany stacji/STOP/PLAY
i TLS po zamykaniu I2S nadal OPEN. Końcowy samotny ESP-ROM bez dalszego
boot reason/backtrace nie pozwala ustalić przyczyny resetu. Kod bez zmian,
nie wykonywano nowego build/testów firmware ani flash; diff check PASS.
