# ADR0031 — działająca konfiguracja WWW, STA i import playlisty

Data: 2026-10-31. Status: implementacja/build/testy hostowe PASS;
odbiór na ESP32-S3 OPEN. Rozwija ADR0030 i zastępuje ograniczenie read-only ADR0027.

## Kontrakt i odpowiedzialności

Własne formularze StreamOO obsługują do 8 profili, kolejność/przenoszenie,
usuwanie i sieci otwarte. Odczyt zwraca SSID i has_password, nigdy hasło.
Pominięcie password zachowuje sekret wyłącznie przy identycznym zapisanym SSID;
pusty string jawnie wybiera sieć otwartą. SSID 1–32 bajtów, hasło 8–63 bajtów
albo puste, poprawny UTF-8, bez kontroli i duplikatów SSID.

network jest jedynym ownerem Wi-Fi. HTTP przekazuje kopię listy kolejką,
nie czeka na łączenie. Worker próbuje profile kolejno, do 15 s na profil
(do IPv4, również błędne hasło/nieobecny AP); przechodzi do następnego.
Nową listę zapisuje w jednym wersjonowanym/checksummowanym blobie NVS dopiero
po połączeniu i commit. Błąd zachowuje poprzedni blob i uruchamia ponowne
próby starej listy. Pusta lista zapisuje świadome usunięcie i utrzymuje AP.
Po restarcie próbuje zapisanej listy. Retry 1/2/4/8/15 s; utrata połączenia
nie powoduje automatycznego ujawnienia AP, restart daje okno konfiguracji.
AP wyłącza się 5 s po STA/IP, umożliwiając poll strony co 2 s. OLED po
wyłączeniu AP pokazuje SSID i nowy URL, przy utracie IP stan ponawiania.
Odtwarzacz pozostaje wyłączony przez cały boot konfiguracji, także po STA;
odtwarzanie internetowe, mDNS, skaner SSID i roaming nie są tym etapem.

configuration udostępnia czyste modele i task-only NVS/LittleFS.
Portal zna callbacki ownera, nie otrzymuje klucza AP jako części statusu.

## Autoryzacja konfiguracji

Wpisanie istniejącego klucza AP potwierdza zapis/odczyt prywatnej konfiguracji:
HTTP Basic z użytkownikiem setup. Klucz pochodzi z prywatnego procesu build,
nie z repo, nie jest zwracany przez HTTP ani logowany. Przeglądarka przekazuje
go w nagłówku żądania; nie umieszcza go w URL/localStorage/sessionStorage.
Dodatkowy nagłówek X-StreamOO-Request:1 i kontrola Origin względem Host dla
mutacji; brak CORS. Osiem błędów autoryzacji/minutę blokuje kolejne próby do
końca okna. Błędne/odrzucone żądania zamykają sesję (ESP_FAIL po odpowiedzi),
bez odczytywania dowolnie dużego odrzuconego body. Limity body i timeouty.

To autoryzacja bieżącego lokalnego setupu przez istniejący klucz, nie wybór
niezależnej administracji dla przyszłej aplikacji. D18 pozostaje OPEN dla
produkcyjnego API/LAN: HTTP Basic nie szyfruje transportu, NVS nie jest
szyfrowany. Nie wystawiać tego setupu przez przekierowanie portów do Internetu.

## Playlista i trwałość

Własne API (bez kopiowania yoRadio):
- GET /api/v1/wifi/profiles; POST /api/v1/wifi/config (JSON, 202).
- POST /api/v1/stations/import/preview oraz /api/v1/stations/import
  (surowe body text/plain, bez multipart).
- GET /api/v1/stations/import/status (liczniki, bez treści katalogu).

Tabulatory oddzielają nazwę/URL/ovol. Maksimum 256 KiB, 2048 rekordów,
nazwa 128 bajtów, URL HTTP(S) 2048 bajtów, ovol signed32 zachowane bez
interpretowania jako gain. Brak URL credentials. Walidacja całego body
w PSRAM, normalizacja kopii żądania: BOM, CRLF i U+200B na początku URL.
Podgląd raportuje liczbę oczyszczonych URL i powtórzeń; duplikaty zachowane.
Osobny przycisk zatwierdza zastąpienie. Commit ponownie waliduje body.
Błędny/przerwany upload nie zmienia katalogu. Plik tymczasowy → fflush/fsync/
fclose → atomowe rename w LittleFS; poprzedni plik pozostaje przy błędzie.
Podgląd nie zapisuje flash. Import nie uruchamia odtwarzania ani nie podłącza
jeszcze katalogu do syntetycznej listy OLED; stable ID i player nadal OPEN.

Custom partitions.csv zachowuje istniejące adresy/rozmiary NVS/PHY/app;
dodaje storage 1 MiB na 0x190000. Wymaga pełnego zwykłego flash tablicy
partycji, bez erase i bez obrazu FS. Pierwszy format tylko całkowicie pustej
partycji (0xFF), z markerem NVS init=1 przed formatem, init=2 po mount.
Przerwane pierwsze formatowanie może zostać wznowione. Uszkodzony niepusty
FS z zakończoną inicjalizacją nie jest automatycznie formatowany.

## Zależności i dowody

Nowe wersje dokładnie przypięte: joltwallet/littlefs 1.20.4 (adapter MIT,
rdzeń BSD-3-Clause), espressif/cjson 1.7.19~2 (MIT). Przegląd manifestów,
API lokalnych nagłówków i licencji przed integracją, native ESP-IDF6.1 build PASS.
Licencje zachowane w licenses i THIRD_PARTY_NOTICES.md.
Źródła przeglądu: https://components.espressif.com/components/joltwallet/littlefs/versions/1.20.4
oraz https://components.espressif.com/components/espressif/cjson/versions/1.7.19~2/readme.

221 testów hostowych PASS: modele, uszkodzenia/truncation NVS, kolejność,
retencja sekretów, limity/UTF-8, HTTP auth/Origin/timeouty/częściowy recv,
preview/commit, restart katalogu, błąd rename i ochrona przed formatowaniem.
Testy persistence używają atrap NVS/partycji i realnych plików hosta;
nie dowodzą odporności fizycznej flash na utratę zasilania.
Chromium 390px: błędne/poprawne uwierzytelnienie, dodanie/przeniesienie sieci,
wysyłka JSON, preview/commit/reset pliku PASS na syntetycznych backendach.
Prywatny CSV sprawdzony parserem firmware na kopii w RAM:
267 rekordów, 11 normalizacji URL, 3 duplikaty; oryginał niezmieniony,
nieśledzony, brak kopii w repo/build. Dane prywatne nie trafiły do testów.

OPEN: odbiór STA/DHCP/AP-off i reboot na PCB, budżet heap/stosów przy uploadzie,
fizyczne przerwanie zasilania podczas zapisu, docelowy player/katalog/ID i auth LAN.
