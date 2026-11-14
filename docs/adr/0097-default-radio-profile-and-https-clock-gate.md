# ADR0097 — zwykły profil radia i start HTTPS po ustawieniu UTC

Data: 2026-11-01. Status: host/build PASS, odbiór nowego startu na S3 OPEN.

## Decyzja i zakres

Użytkownik zlecił utrwalenie profilu sprawdzonego w ADR0095–0096 oraz
oczekiwanie na czas wyłącznie dla HTTPS. HTTP ma startować bez czekania
na NTP. Zapytał również o szybszy NTP, np. równoległe trzy zapytania.

Normalny sdkconfig.defaults wybiera dr_flac, hardware AES/SHA, przygotowanie
crypto przed I2S oraz TCP23040/mailbox18. Kconfig domyślnie wybiera dr i
early init (gdy sprzętowe crypto jest dostępne). Backendy porównawcze oraz
software crypto pozostają wybieralne. W tym workspace uaktualniono tylko
odpowiednie opcje prywatnego sdkconfig i sdkconfig.flac-dr; GPIO, sekrety,
partycje i pozostałe ustawienia zachowane. Prywatnych plików nie commitujemy.

Zwykłe `idf.py build` / `idf.py -p /dev/ttyUSB0 flash` używa teraz tego
profilu w tym workspace, bez wrappera. sdkconfig.defaults dotyczy generowania
konfiguracji; w innym, starszym checkout istniejący sdkconfig ma pierwszeństwo
i wymaga jawnej aktualizacji w menuconfig. Nie wymuszamy nadpisania osobistej
konfiguracji przy każdym uruchomieniu CMake. Bufory audio, progi, DSP, I2S
i PSRAM/lwIP allocation policy pozostają jak w ostatnim dobrym teście.

## HTTPS a zegar

Kontrola znajduje się w readerze, przed otwarciem każdego docelowego HTTPS:
adres główny, przekierowanie HTTP→HTTPS, M3U/PLS, HLS manifest i segment.
HTTP nie czyta zegara ani nie czeka. Walidacja UTC2020..2099 jest spójna
z zakresem dotychczasowej projekcji zegara i callbacku SNTP. Zimny start
z epoch0 czeka na SNTP. Zachowany wiarygodny zakresem RTC może zostać
użyty od razu, bez wymagania ponownego callbacku SNTP po resecie.
Sam zakres nie dowodzi dokładności zegara; X.509 nadal sprawdza daty.

Reader czeka najwyżej30s, z uśpieniem50ms i sprawdzeniem anulowania. Przy
braku czasu kończy z ESP_ERR_TIMEOUT, reason=clock_timeout i stage=time_sync,
bez próby TLS; owner może ponowić sesję dotychczasowym backoff. STOP/zmiana
stacji nadal anulują reader bez blokowania UI lub fill. Nie ma alokacji
ani oczekiwania w ISR/I2S. Przy pierwotnym HTTPS oczekiwanie poprzedza nawet
esp_http_client_init, więc nie rezerwuje wtedy obiektów klienta TLS.

Log: `HTTPS waiting for UTC (SNTP); HTTP remains available`, później
`HTTPS UTC ready after ...ms`. Nie loguje URL, query ani danych prywatnych.
Certyfikaty, hostname, CA bundle, kontrola dat i zakaz downgrade zachowane.
Zegar nie dostaje sztucznej daty z kompilacji ani ostatniego zapisu NVS.

## SNTP: szybszy start i trzy serwery

Sprawdzono API/implementację lokalnego ESP-IDF6.1 oraz
[dokumentację ESP-NETIF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/network/esp_netif_programming.html).
Istniejące esp_netif/lwIP, licencje Apache-2.0/BSD według SDK; bez nowej
biblioteki i bez kopiowania implementacji NTP.

Poprzednia konfiguracja miała losową zwłokę0..5000ms przed pierwszą prośbą.
Wyłączamy CONFIG_LWIP_SNTP_STARTUP_DELAY; usługa rusza po otrzymaniu IP STA,
asynchronicznie. CONFIG_LWIP_SNTP_MAX_SERVERS=3, serwery0/1/2.pool.ntp.org
zgodnie z [opisem NTP Pool](https://www.ntppool.org/en/use.html). Nie wpisujemy
stałych adresów IP i nie skracamy okresowego odświeżania1h.

Natywny [klient lwIP SNTP](https://github.com/espressif/esp-lwip/blob/master/src/apps/sntp/sntp.c)
używa jednego bieżącego serwera i przechodzi do kolejnego po błędzie lub
timeout, a nie wysyła trzech równoległych zapytań. Lokalny SNTP_RECV_TIMEOUT
wynosi15s; DNS może dodać własne opóźnienie. Trzy wpisy poprawiają dostępność,
nie gwarantują odpowiedzi najszybszego z trzech. Równoległy wyścig wymagałby
osobnego klienta/protokołu, jego walidacji i polityki aktualizacji zegara;
nie dodajemy go do tej zmiany ani nie udajemy równoległości przez wielokrotny
init globalnego SNTP. Usunięcie zwłoki może skrócić początek o dotychczasowe
wylosowane0..5s; faktyczny efekt DNS/UDP na płytce pozostaje do zmierzenia.

Starsze prywatne sdkconfig backendów z limitem1 nadal kompilują się z jednym
serwerem; nowy normalny profil i aktualny dr mają trzy. Log podaje faktyczną
liczbę. Nie zmieniamy globalnego SDK ani jego timeoutów.

## Walidacja i odbiór

Test rzeczywistego readera: HTTP przy UTC0 bez opóźnienia, HTTPS czeka przed
open, HTTP→HTTPS czeka dopiero po przekierowaniu, anulowanie przed alokacją
klienta, timeout bez TLS, niepoprawny przyszły UTC, zachowany poprawny RTC,
HLS manifest i ponowna kontrola przed zasobem HLS. Test sieci sprawdza trzy
nazwy i nieblokujący config/callback, istniejący test defaultów obejmuje profil.

472 testy hostowe PASS;33 regresje readera ASan/UBSan PASS (LSan off).
Zwykłe idf.py build/ESP-IDF6.1 PASS, binarka1574288B. Nagłówek generowany
potwierdza dr/hardware AES/SHA/early init/TCP23040/mailbox18/SNTP3/no-delay;
etykieta backendu binarki również sprawdzona. Agent nie flashował.
Osobne reconfigure ze świeżym SDKCONFIG w /tmp potwierdziło wybór z samych
defaults: dr/hardware/early init/TCP23040/18/SNTP3/no startup delay. To
weryfikacja generacji konfiguracji, nie drugi build ani odbiór sprzętowy.
Na płytce: standardowy flash, reset z HTTPS i obserwacja SNTP /
HTTPS UTC ready / pierwszego połączenia; następnie HTTP przy nieustawionym
czasie (np. blokada UDP123) i przejście do HTTPS. HTTP powinno grać, HTTPS
czekać/zgłaszać clock_timeout. Sprawdzić STOP podczas oczekiwania i szybki
wybór HTTP; błędny certyfikat nadal musi kończyć TLS błędem po ustawieniu
czasu. Odsłuch nie jest obecnie dostępny; weryfikujemy logi i ciągłość
liczników. Profil pamięci pozostaje do obserwacji przy WWW/przełączaniu.
