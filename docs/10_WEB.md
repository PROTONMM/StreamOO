# 10 — Nowe WWW i REST API StreamOO

Instrukcja bieżącego panelu dla użytkownika:
[USER_GUIDE.md](USER_GUIDE.md#dostęp-do-panelu-www).
W LAN import jest w zakładce Playlista; w AP na stronie konfiguracji.

## Głośność podczas przesuwania — ADR0111 / 2026-11-14

Suwak WWW wysyła komendy już na input, bez czekania na puszczenie.
Najwyżej10 żądań/s, tylko jedno w toku; oczekuje wyłącznie najnowsza
wartość. Change/blur/cancel kończą edycję bez gubienia końcowej wartości
ani duplikowania przyjętej. Suwak pozostaje aktywny w czasie HTTP,
polling nie przestawia go podczas przeciągania. Błąd/offline usuwa kolejkę.

27 testów portalu PASS, w tym4 nowe testy timingu/wolnego HTTP/błędów.
Chromium potwierdził zmianę stanu serwera przed puszczeniem suwaka oraz
pozostałe przepływy AP/LAN PASS. Node/diff PASS. Native ESP-IDF6.1 build
PASS:0x177400B,90% app wolne. Bez flash agenta. Odsłuch i fizyczny dotyk
Androida pozostają OPEN. Backend/rampa/NVS/I2S bez zmian.

## Zwarty odtwarzacz WWW i nawigacja stacji — ADR0110 / 2026-11-14

Usunięto kartę sieci i zwykły komunikat połączenia. Zakładki telefonu28px,
sterowanie36px w jednym rzędzie: poprzednia/PLAY/STOP/następna jako SVG,
suwak i wartość volume. Lista zajmuje około2/3 ekranu telefonu; pasek
wyszukiwania/licznika ma jeden wiersz. Etykiety edytora są ukryte wizualnie,
zachowane dla czytnika ekranu; pola i przycisk usuwania są bardziej zwarte.
Nawigacja po pełnym katalogu z zawijaniem, niezależna od filtra. Wspólny
wybór/CAS, blokada żądań równoległych oraz obsługa pustej/jednej stacji.

23 testy portalu PASS, Chromium: pełne przepływy i nowe przyciski PASS;
6 podglądów/układów320..1280px PASS, w tym edytor oraz pomiar listy>=66%
wysokości pionowego okna telefonu. Node/diff PASS. Native ESP-IDF6.1
build PASS:0x176a80B,90% app wolne. Bez flash agenta; fizyczny Android
oraz sterowanie rzeczywistym radiem OPEN. Szczegóły: ADR0110.

## Zwarty panel WWW, motywy i format audio — ADR0109 / 2026-11-14

Usunięto duży nagłówek „Twoje radio”. Sterowanie, kodek/bitrate/próbkowanie
oraz IP od razu dostępne; PC ma dwie kolumny, telefon jedną. Lista wszystkich
stacji bez stron wypełnia dostępną wysokość. Ustawienia i import rozwijane.
Motywy jasny/ciemny/systemowy, wybór zapamiętany lokalnie w przeglądarce.
Przy wykrytej zmienności panel pokazuje tylko „Zmienny”; estymacja liczby
ma≈, brak formatu/STOP/offline — kreski. API rozszerzone o jawny snapshot
formatu. Detektor bazuje na trzech istotnych zmianach okien>=2s audio,
nie na tempie sieci; ograniczenia i progi opisane w ADR0109.

499 testów hostowych PASS, bez skipów. Native ESP-IDF6.1/esp32s3 build
PASS:0x175f60B,90% partycji app wolne. Node syntax, GPIO/platform i diff
PASS. Szczegóły i polecenia odtworzenia: ADR0109.

Chromium: przepływy AP/LAN i pięć układów/motywów320..1280px PASS,
2048 stacji, wyszukiwanie, zachowanie przewijania, tryb systemowy i blokada
localStorage PASS. Podglądy obejrzano. Bez flash agenta; fizyczny Android
oraz bitrate na rzeczywistym radiu pozostają OPEN.


Nowe logo StreamOO: wspólna geometria SVG/4bpp, [ADR0108](adr/0108-streamoo-wordmark.md).

## Odtwarzacz — lista przewijana / ADR0107 / 2026-11-14

Wszystkie stacje są dostępne w jednym przewijanym oknie bez stron,
z wyszukiwaniem w całym katalogu i licznikiem wyników. Zmiana aktywnej
stacji nie odtwarza DOM ani nie resetuje przewinięcia. Edytor zachowuje
odrębną paginację. REST i zawartość zapisanej listy bez zmian.

Stan implementacji 2026-10-31: Formularze sieci i playlisty działają bez dodatkowego logowania na polecenie użytkownika (ADR0032, zastępuje auth ADR0031). Automatyczny odczyt ustawień po otwarciu strony. Kontrakt/limity ADR0031; polityka przyszłego API osobno; OTA wyłączone decyzją użytkownika (ADR0055).

**USTALONE:** całkowicie własny serwer WWW/API, bez kompatybilności endpointów z yoRadio. ESP-IDF `esp_http_server` + pliki frontendu. Dostęp przez AP przy pierwszym uruchomieniu i przez unikalny mDNS/IP po połączeniu do LAN.

## Proponowane endpointy (nie kontrakt zamknięty)
| Metoda | Ścieżka | Funkcja |
|---|---|---|
| GET | `/api/v1/status` | zdrowie, stan, IP/wersja |
| GET | `/api/v1/player` | status i metadata |
| POST | `/api/v1/player/actions` | play/stop/select station |
| PUT | `/api/v1/player/volume` | wartość 0..100 |
| GET | `/api/v1/stations` | lista |
| POST | `/api/v1/stations` | dodaj |
| PUT/DELETE | `/api/v1/stations/{id}` | edytuj/usuń |
| POST | `/api/v1/stations/import` | upload, preview/commit |
| GET | `/api/v1/stations/export` | eksport yoRadio |
| GET | `/api/v1/wifi/scan` | sieci |
| POST | `/api/v1/wifi/config` | zmiana Wi-Fi |
| GET/PUT | `/api/v1/config` | ustawienia dozwolone runtime |

API ma osobne DTO od struktur aplikacji, waliduje JSON/rozmiary/wysyłkę plików i pozostawia lokalną konfigurację dostępną bez osobnego logowania (ADR0032). Zachowaj kontrolę Origin i custom header dla mutacji oraz odporność na niepoprawne długości pól; nie stosuj CORS `*` na administracji.

## Frontend
Responsywny prosty panel z zakładkami Player / Stations / Wi-Fi / System. Import z podglądem błędów; sortowanie drag-and-drop i manualna edycja URL, obsługa klawiatury. UI web nie powinno mieć logiki dekodowania ani zależeć od API wewnętrznych sterowników.

## Testy
API contract, nieautoryzowane modyfikacje, upload uszkodzonego pliku, edycja stacji podczas odtwarzania, pełny provisioning offline.

## Pierwszy zakres09b — odczyt

Własne offline HTML/CSS/JS i GET/api/v1/status na esp_http_server6.1.
Status sieci/audio pochodzi z jawnego gettera aplikacji, nie z syntetycznego
HOME. Bez metod modyfikujących, formularzy credentials i CORS '*'.
Panel nie pokazuje kluczy; hasło AP wyłącznie na OLED na polecenie użytkownika.
Auth administracji i pełna konfiguracja Wi-Fi pozostają OPEN przed dodaniem
zapisu. [ADR0027](adr/0027-readonly-portal-and-ap-oled.md).

## Przyszła aplikacja telefoniczna

Użytkownik ustalił sterowanie radiem z aplikacji telefonu przez API.
WWW i aplikacja korzystają ze wspólnego wersjonowanego kontraktu i ownera
odtwarzania, bez oddzielnej implementacji playera. Obecnie gotowy tylko
odczyt statusu; akcje sterowania, uprawnienia/auth i kontrakt nadal OPEN.
Platformy/forma aplikacji i sposób aktualizacji stanu zostaną ustalone później.
Nie deklarować działającego sterowania telefonem na podstawie portalu09b.
ADR0029; blokada odtwarzania w AP dotyczy także przyszłego klienta API.

## Uzupełnienie konfiguracji2026-10-31

WWW ma umożliwić zapis/edycję kolejności kilku profili Wi-Fi oraz upload
playlist.csv. Przyjmujemy kolejność profili jako priorytet pierwszej dostępnej
sieci. Import stacji z walidacją i podglądem, następnie atomowym zatwierdzeniem;
plik wejściowy prywatny, nie część repo/build. Weryfikacja formatu ADR0030.
Obecny portal tylko odczytuje status; formularze i operacje zapisu nadal OPEN.

## Zegar / zakładka Konfiguracja — ADR0047

Nowe zakładki Status i Konfiguracja. Konfiguracja zawiera Wi-Fi, import
i Data i czas z selectem strefy i zapisem NVS. GET/POST /api/v1/clock,
POST JSON {"timezone":"Europe/Warsaw"}, limit128B i whitelist. Origin/
custom header bez zmian, bez dodatkowego loginu. Błąd commit503 zachowuje
poprzednią aktywną strefę. Pierwszy katalog8 stref europejskich/UTC.

## Sterowanie playerem — ADR0050

Status ma Odtwarzaj/Zatrzymaj i suwak volume0..100. POST/player/control
pod /api/v1, JSON action play/stop/volume,202 przyjęte. Wspólny owner
z encoderem, idempotentny PLAY/STOP i absolute volume. Tylko STA/AP-off,
bez dodatkowego loginu; Origin/header/limity. Wybór stacji przez WWW
i aplikacja telefonu nadal OPEN. Procedura WEB_PLAYER_BRINGUP.

## AP setup-only / LAN player i edytor — ADR0051

AP wyłącznie Wi-Fi/import/zegar; po STA/AP-off domyślnie odtwarzacz
z klikanymi stacjami. Playlista ma ręczne add/edit/delete, save, export,
preview/import i clear z potwierdzeniem. Konfiguracja Wi-Fi/zegara w LAN
nadal osobno. Endpointy stations/list/export/replace/select przez wspólny
katalog, generacje string i CAS. Nowe operacje player/editor odrzucane w AP.
Procedura WEB_LIBRARY_BRINGUP, dane prywatne nie trafiają do repo.

ADR0070: zatwierdzony upload playlist.csv dopisuje stacje do obecnego
katalogu. Przycisk clear pozostaje jedyną operacją usuwającą całą listę;
zapis edytora pozostaje świadomym zastąpieniem całej wersji katalogu.

## Ustawienia obsługi i startu — ADR0059

LAN/Konfiguracja: czas listy1..120s, auto-wybór lub powrót bez zmiany,
start STOP/autoplay ostatniej stacji oraz własna nazwa `.local`. W AP
ten panel ukryty; nadal tylko Wi-Fi/import/strefa. GET/POST
`/api/v1/settings`: dokładnie `list_timeout_s` (integer),
`list_autoselect` (bool), `autoplay` (bool), `hostname` (DNS label lub
pusty). POST512B, JSON, Origin/header i walidacja, zapis NVS przed200;
błąd flash503 bez publikacji nowej konfiguracji. Edytor zapisuje
`application/vnd.streamoo.stations+tsv`: ID16hex/TAB/nazwa/TAB/URL/TAB/ovol,
ID0000000000000000 tylko dla nowej pozycji. Endpoint replace nadal
przyjmuje starszy3-polowy text/plain; eksport/import użytkownika bez ID.
