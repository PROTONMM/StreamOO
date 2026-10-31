# 10 — Nowe WWW i REST API StreamOO

Stan implementacji 2026-10-31: Formularze sieci i playlisty działają bez dodatkowego logowania na polecenie użytkownika (ADR0032, zastępuje auth ADR0031). Automatyczny odczyt ustawień po otwarciu strony. Kontrakt/limity ADR0031; polityka przyszłego API/OTA osobno.

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
