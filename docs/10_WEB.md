# 10 — Nowe WWW i REST API StreamOO

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

API ma osobne DTO od struktur aplikacji, waliduje JSON/rozmiary/wysyłkę plików i zapewnia kontrolę uprawnień (rozdziel tryb provisioning i tryb zarządzania). Zaplanuj odporność na CSRF, przechwycenie sesji LAN, brute force i niepoprawne długości pól; nie stosuj CORS `*` na administracji.

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
