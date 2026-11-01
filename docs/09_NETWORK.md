# 09 — Wi-Fi provisioning i mDNS

ADR0097: normalny profil SNTP ma trzy serwery0/1/2.pool.ntp.org i nie ma
losowej zwłoki startowej0..5s. To fallbacki sekwencyjne, nie trzy zapytania
jednocześnie; lokalny timeout odpowiedzi15s. HTTP nie czeka na zegar,
reader wstrzymuje wyłącznie HTTPS do poprawnego zakresem UTC (max30s,
anulowanie co50ms), także dla redirect/playlist/HLS. Daty certyfikatów
sprawdzane nadal. Odbiór rzeczywistego czasu startu na S3 OPEN.

ADR0095: profil wrappera `--tcp-window 23040` zwiększa domyślne okno RX
TCP5760→23040B i mailbox6→18 (MSS1440), weryfikuje sdkconfig.h przed flash.
Bez flagi zachowuje prywatną konfigurację backendu. Hipoteza ograniczenia
WAN, nie potwierdzona naprawa; koszty pamięci/odsłuch S3 OPEN. Bez zmian
ring/PCM/preroll i bez modyfikowania globalnego SDK.

ADR0090: po starcie drivera radio ustawia WIFI_PS_NONE, aby ograniczyć
opóźnienia odbioru względem domyślnego modem sleep. Potwierdza getterem;
błąd tylko ostrzega i nie blokuje sieci/AP. Większy pobór energii Wi-Fi,
bez zmian TCP/buforów/mocy TX. Odbiór wpływu na FLAC pozostaje OPEN.

Stan implementacji 2026-10-31: 09e/ADR0036 — STA przed AP; do 8 uporządkowanych profili NVS, worker STA, fallback i AP-off po IPv4. Odbiór sprzętowy OPEN. ADR0031 i NETWORK_BRINGUP.

**USTALONE:** przy pierwszym uruchomieniu bez danych STA urządzenie tworzy SoftAP z unikatową nazwą `StreamOO-XXXXXX` opartą na MAC. **Hasło AP jest stałe, wspólne dla wszystkich urządzeń** (przekazywane poza repo); WPA2-Personal. Portal WWW pod `http://192.168.4.1`; po pomyślnym połączeniu z Wi-Fi STA wyłącz SoftAP. Użytkownik może później zmienić Wi-Fi przez stronę WWW.

## Kilka zapisanych sieci (USTALONE2026-10-31)

Konfiguracja WWW zapisuje uporządkowaną listę profili SSID/hasło. Kolejność
listy jest przyjętym priorytetem: przy wyborze połączenia próbujemy pierwszą
dostępną sieć, a po nieudanym uwierzytelnieniu/DHCP kolejną. Nie wybieramy
wyłącznie najmocniejszego RSSI i nie kasujemy pozostałych profili. Sukces
wymaga STA/IP; dopiero wtedy AP off. Trwałość w NVS, bez ujawniania haseł
w GET/logach/repo. Limity profili, hidden SSID i polityka aktywnego roamingu
OPEN. Zapis i STA wdrożone w ADR0031; kolejność startu uaktualnia ADR0036.

## Przepływ
- Brak credentiali → SoftAP → skan SSID, wybór, hasło → tymczasowy STA test → dopiero po powodzeniu zapis do NVS → `WIFI_MODE_STA`, AP wyłączony.
- Zła konfiguracja → portal AP pozostaje aktywny, stary profil pozostaje dostępny; strona komunikuje błąd.
- Przy zmianie Wi-Fi przez WWW: przejściowe rozłączenie przeglądarki jest spodziewane; procedura rollback do ostatnich działających danych po nieudanej zmianie.
- Krótki zanik STA nie uruchamia AP natychmiast; najpierw próba całej zapisanej listy, AP dopiero po jej niepowodzeniu. Awaryjne uruchomienie AP w menu jako przyszły mechanizm (gest/menu do implementacji).

## mDNS
- Host domyślny `streamoo-xxxxxx.local` na bazie unikalnego sufiksu MAC, **nie** wspólny `streamoo.local`.
- Użytkownik może zmienić nazwę przez WWW; zapis w NVS, walidacja DNS hostname i obsługa konfliktów.
- Reklamuj HTTP przez DNS-SD `_http._tcp`; po zmianie Wi-Fi odśwież usługę.

## Bezpieczeństwo
Wspólne hasło AP zwiększa ryzyko nieautoryzowanego dostępu; ogranicz czas/warunki aktywacji AP, nie ujawniaj hasła w logach, lokalny panel pozostaje bez dodatkowego logowania na polecenie użytkownika (ADR0032). Preferuj proces build/secrets dla wartości wspólnej, bez umieszczania jej w Git.

## Pierwszy zakres09a — test SoftAP

Natywny AP WPA2 i DHCP w osobnym zadaniu; hasło podawane lokalnie przez
`tools/configure_network.py`, plik poza repo. Bez klucza Wi-Fi pozostaje
wyłączone. Na tym etapie brak portalu/STA/mDNS i zapisu credentials do NVS;
pełny przepływ powyżej pozostaje wymaganiem. Instrukcja:
[NETWORK_BRINGUP](NETWORK_BRINGUP.md), [ADR0026](adr/0026-secured-softap-bringup.md).
Nowy profil partycji SDK1500KiB zachowuje położenie NVS/PHY/factory.
Odbiór RF/DHCP i współpracy z audio OPEN.

## Zakres09b — portal odczytu i ekran AP

Statyczny portal HTTP192.168.4.1 i GET/api/v1/status, bez zapisów ustawień.
OLED podczas AP wyświetla SSID i pełne hasło, zgodnie z poleceniem użytkownika.
Stałe hasło nadal wbudowane z prywatnego buildu, niewysyłane przez HTTP/logi.
Użytkownik potwierdził AP off po udanym STA z IP; przejście to będzie wdrożone
wraz z zapisem/testem STA, nie po samym dołączeniu klienta AP.
[ADR0027](adr/0027-readonly-portal-and-ap-oled.md). Odbiór09b OPEN.

Nowsza decyzja: konfiguracja AP bez odtwarzania. Boot pomija cały testowy
pipeline audio, enkoder w AP nie steruje playerem, portal pokazuje audio off.
To zastępuje eksperymentalną współpracę lokalnego AAC z AP. MicroSD później,
niezaimplementowane; przyszłe dynamiczne STA→AP wymaga kontrolowanego STOP.
[ADR0028](adr/0028-ap-setup-without-audio.md).

09d: po STA i AP-off OLED wraca do HOME z adresem IP w trzeciej linii; dostęp WWW lokalnie bez dodatkowego hasła. ADR0032.

06i: niezależny reader HTTP/HTTPS/ICY używa stałej sesji przy krótkim underrun; owner player zamyka ją przy STOP/braku STA/AP-active. SNTP po pierwszym IP dla czasu TLS, bez wyłączania weryfikacji certyfikatów. ADR0034.

## 09e — STA najpierw (ADR0036)

Wi-Fi startuje w trybie STA. Każdy profil ma limit15s na połączenie/IP;
WIFI_FAST_SCAN kończy szukanie po znalezieniu zgodnego SSID. Priorytetem
jest kolejność profili, nie porównanie RSSI wszystkich sieci. Brak sztucznego
opóźnienia przed pierwszą próbą. Rozłączenie aktywnej próby czeka na ACK
maksymalnie500ms; zakończony DISCONNECTED nie wymaga kolejnego rozłączenia.
Sukces startu nie uruchamia AP ani jego ekranu. Dopiero brak profili lub
niepowodzenie całej listy włącza AP i portal konfiguracji. Nieudana zmiana
z LAN najpierw przywraca poprzednią listę; AP dopiero gdy ona też zawiedzie.
Jeśli AP już działało, po udanym IP zachowujemy5s na zakończenie odpowiedzi
WWW przed AP-off. mDNS i aktywny roaming pozostają OPEN.

ADR0044: RSSI w snapshotach sieci. Worker odczytuje po połączeniu i co1s,
bez skanowania/zmiany sieci; brak STA lub błąd odczytu unieważnia wynik.
Dodatnie wartości SDK clamped do0, -128 nieznane. Dane dla ikony HOME,
bez połączenia renderera z SDK.

ADR0047: sync_cb SNTP ustawia network.time_synced dopiero dla wiarygodnego
zakresu UTC2020..2099. Flaga nie znika przy utracie STA, zeruje się na reboot
lub nieważnej korekcie. Strefa dotyczy tylko projekcji UI, nie czasu TLS.

## 09f — nazwa LAN (ADR0052)

Dodano domyślną nazwę MAC `streamoo-xxxxxx.local` i ogłaszanie HTTP80
wyłącznie STA/AP-off, po błędzie retry30s. Dokładny komponent Espressif
mDNS1.14.0, Apache2.0. Konfiguracja własnej nazwy przez WWW/NVS nadal OPEN.
Procedura sprzętowa: [MDNS_BRINGUP](MDNS_BRINGUP.md).

ADR0054: reader rozwiązuje URL M3U/PLS przed uruchomieniem dekodera,
z limitami dokumentu/zagnieżdżeń, względnymi URL i zachowanym zakazem
HTTPS→HTTP. Katalog i ID przechowują oryginalny adres. HLS osobno OPEN.

ADR0057: natywna weryfikacja cross-signed CA bundle włączona dla
alternatywnych ścieżek do już zaufanych rootów (RNŚAAC+). Pełny bundle,
sprawdzanie podpisów/hostname pozostają; HAVE_TIME_DATE teraz jawnie y
(wcześniej SDK default n), z istniejącym SNTP. Odbiór na S3 OPEN.

## Własna nazwa LAN — ADR0059

Konfiguracja WWW/NVS pozwala wybrać etykietę DNS1..63 znaków: małe litery,
cyfry, łącznik wewnątrz. Puste pole przywraca nazwę MAC. Worker sieci
restartuje tylko mDNS po zmianie; Wi-Fi i audio działają dalej. Konflikt
obsługuje dotychczasowy callback SDK; log pokazuje faktyczny adres.

## HLS — ADR0060

Wcześniejsze jawne odrzucanie HLS zastępuje reader nieszyfrowanego live/VOD:
master/media, AAC/MP3 packed audio oraz MPEG-TS przez istniejący codec2.6.2.
Nie obejmuje AES/DRM/fMP4/BYTERANGE/LL-HLS; discontinuity po starcie kończy
sesję/retry. Limity16KiB/32 segmenty, osobny manifest PSRAM, reader16KiB,
TLS i ring256KiB zachowane. Testy hostowe/publiczne próbki PASS, odsłuch
S3 OPEN. Instrukcja: [HLS_BRINGUP](HLS_BRINGUP.md), `playlist.hls.csv`.
Generator extended fixtures dodaje3 lokalne HLS: AAC-TS/MP3-TS/AAC-packed.

ADR0065: reader posiada pełny bieżący URL wraz z query (lsid/podpisy),
ponieważ pinned SDKget_url zwracał sam scheme/host/port/path. Dotyczy
HLS reload, M3U/PLS i manualnych Location. Resolver/limity/no-downgrade
zachowane; tracker nie dodaje logowania query ani osobnego zapisu w NVS. Hostowa
reprodukcja RadioMast i testy PASS; odbiór wielokrotnych reload S3 OPEN.
