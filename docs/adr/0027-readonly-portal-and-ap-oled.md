# ADR0027 — portal statusu i dane AP na OLED

Data:2026-10-31. Status: wdrożone; odbiór09b na S3 OPEN.

## Wymagania i decyzja

Użytkownik potwierdził stałe hasło AP wbudowane w firmware, wyświetlanie
SSID i hasła na OLED w trybie AP oraz wyłączenie AP po połączeniu STA.
Stały klucz nadal pochodzi z prywatnego pliku procesu budowania i jest
niezmienny runtime; nie trafia do repo/logów. Ta decyzja nie ustala osobnego
uwierzytelniania administracyjnego. Dlatego09b dodaje wyłącznie portal odczytu;
zapis Wi-Fi, STA i przejście AP→STA pozostają kolejnym etapem. Docelowo AP
wyłączamy po udanym STA z adresem IP, nie po dołączeniu klienta do SoftAP.

## Architektura

Nowy komponent portal: esp_http_server dostarczony z ESP-IDF6.1,
Apache2.0 (publiczny nagłówek2018–2026 Espressif). API start/stop, rejestrację
URI, wysyłanie statycznych zasobów i limity sprawdzono w lokalnym SDK.
Bez zewnętrznych pakietów, skopiowanego kodu/API yoRadio i CDN.

HTTP task priorytet2, stos6144B, max4 sockety, LRU eviction, recv/send timeout3s.
Własne statyczne HTML/CSS/JS we flash. Tylko GET /, /portal.css, /portal.js,
/api/v1/status. Brak jakichkolwiek operacji zmieniających stan. DTO jest jawne:
tryb/SSID/ready/klienci AP, dostępność/testowe audio/running/buffering/volume,
uptime i schema_version1. Getter aplikacji spina sieć i audio przez kopie,
bez zależności portal→network/audio. Nie ujawnia haseł, NVS ani MAC klienta.
JSON ma stały bufor512B; nieprawidłowy snapshot daje503. Whitelist SSID
zapobiega wstawianiu nieescapowanych danych do JSON. Brak CORS '*'.

Network owner wywołuje hooks ready/stopped poza krótką blokadą snapshotu.
Aplikacyjny adapter uruchamia/zatrzymuje HTTP w zadaniu managera sieci.
Network i portal pozostają niezależne: błąd HTTP nie wyłącza AP/audio.
Stop z błędem zachowuje żywy kontekst serwera, nie zwalnia getter/context
przed zakończeniem obsługi. Timeouty/lifecycle SDK na S3 nadal do testu.

Cache-Control no-store, nosniff, referrer no-referrer i CSP self/no framing/
no forms. JS używa textContent, odczytuje status co2s po zakończeniu poprzedniej
próby, abort po3s; brak nakładających się polli i zewnętrznych zasobów.
Dane są jawnie tylko do odczytu. Liczba klientów pochodzi ze snapshotu workera
sieci odświeżanego co10s. To nie captive DNS ani portal automatycznie otwierany
przez telefon; adres podawany ręcznie HTTP192.168.4.1.

## OLED i klucz

Oddzielna zaufana projekcja ap_display udostępnia klucz tylko lokalnemu UI
podczas ap_ready. Publiczny snapshot sieci i DTO HTTP nie mają pola z kluczem.
Renderer ui_ap rysuje SSID, hasło i URL, bez logowania/zapisu. Pełne63 znaki
hasła mieszczą się w dwóch wierszach42+21; bez ucięcia/dodawania separatora.
Cały framebuffer jest czyszczony przed nowym rysunkiem. Gdy AP nieaktywne,
getter zwraca false/zerowy obiekt i UI wraca do HOME. Nie wymuszamy obecnie
niezaimplementowanego STA tylko po to, by ukryć ekran.

W AP ekran konfiguracji ma pierwszeństwo. Enkoder nadal steruje VOL i PLAY/STOP
w trybie A/B, ale gest listy i pressed-rotate są ignorowane; lista nie otwiera
się niewidocznie. Brak sieci/AP off przywraca poprzedni HOME. Renderer czysty C,
bez RTOS/drivera/heap; używa istniejącego fontu, bez nowego importu bitmap.

## Weryfikacja

197 testów hostowych PASS: model/JSON/bounds, realne handlery z shimem HTTP
(read-only routes, nagłówki,503, cleanup start/stop), DOM JS z symulacją fetch,
pełne63 znaki OLED, czyszczenie pozostałości i granice framebufferu. ASan/UBSan
4 przypadki handlerów/modelu PASS (bez LeakSanitizer). Nie jest to hostowy
runtime esp_http_server/FreeRTOS ani próba transportu na płytce.

Chromium headless390px: lokalny serwer z przykładowym statusem, poprawny DOM,
aktualizacja JSON i screenshot sprawdzone. Brak dostępu do prywatnego klucza
w podglądzie. Native buildy/status w STATUS. Nie flashowano za użytkownika.
Odbiór S3: dane AP na OLED, status w przeglądarce i JSON zgodne z enkoderem,
czysty dźwięk i stabilne PCM/heap podczas odświeżania, brak klucza w HTTP/logach.
STA, AP off po IP, auth admin, reconnect i pełne provisioning nadal OPEN.
