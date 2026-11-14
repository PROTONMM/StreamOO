# ADR0051 — konfiguracja AP, odtwarzacz LAN i edytor playlisty

Data:2026-10-31. Status: wdrożone, odbiór sprzętowy OPEN.

## Decyzja użytkownika i widoki

AP: wyłącznie konfiguracja Wi-Fi, import playlisty i strefa czasu. Brak
nawigacji do playera/edytora; brak pobierania nazw/URL stacji w tym trybie.
Po STA i AP-off domyślnie Odtwarzacz: PLAY/STOP/volume, bieżąca stacja,
szukana/paginowana lista, kliknięcie wybiera stację. Informacje o sieci
zwijane, kontrolki odtwarzania pierwsze. LAN ma też Playlista i Konfiguracja
(Wi-Fi/zegar). Import w AP jest w konfiguracji, w LAN przeniesiony do edytora.
Przejścia trybu śledzą status urządzenia; błąd pojedynczego odczytu statusu
nie udaje przejścia w AP, tylko wyłącza sterowanie jako offline.

Edytor: dodawanie ręczne, poprawianie nazwy/URL, usuwanie wierszy, zapis,
eksport CSV, istniejący preview/import, czyszczenie całej listy z potwierdzeniem.
Zmiany wierszy są robocze do Zapisz; Usuń całą listę po potwierdzeniu zapisuje
pusty katalog od razu. Import ostrzega przed utratą niezapisanej edycji.
Export dotyczy zapisanej listy, przy roboczych zmianach jawne potwierdzenie.
Paginacja playera50, edytora25, max2048. Nazwy przez textContent, nie HTML.
Pole ovol zachowane dla istniejących wierszy, nowe mają0; nie dodajemy
pozornej regulacji korekcji, której DSP jeszcze nie interpretuje.

## Wspólna lista i własność

Lista ze snapshotu configuration, ta sama co OLED. HTTP utrzymuje referencję
przez cały GET/eksport, nie czyta flash. Kliknięcie ID/generation przechodzi
przez istniejący player_select, kolejkę i stale guard — bez nowego playera,
URL z klienta ani losowego wyboru. Status API rozszerzony o station_id
jako16 znaków hex; ID/generation w JS są stringami, bez utraty64bit precision.
Aktualna stacja zaznaczana z faktycznego statusu, nie optymistycznie po202.

Edytor zapisuje canonical CSV przez nowy CAS store endpoint, nie przez
duże drzewo JSON. Walidacja dotychczasowym parserem, staging/fsync/rename
i publikacja snapshotu. expected_generation sprawdzane pod mutexem zapisu
przed operacją plikową; konflikt409 nie zmienia pliku. Natywna generacja
ma32bit boot nonce esp_random plus licznik, ograniczając ponowne użycie
wersji po restarcie; nie jest tokenem autoryzacji ani kryptograficzną
gwarancją unikalności. API esp_random sprawdzone w SDK6.1 esp_random.h.
Host testuje deterministyczny licznik; inicjalizacja nonce kompilowana na S3.

Pusty katalog jest trwałym plikiem0B i snapshotem0 stacji. Startup poprawnie
odtwarza pustość. Dotychczasowy import pustego pliku nadal odrzucany —
do celowego czyszczenia służy przycisk edytora. Wyeksportowana niepusta
lista ma3 literalne pola TAB UTF-8: nazwa/URL/ovol, bez nagłówka/quotingu.
Zachowane kolejność, duplikaty i wartości ovol; eksport kanonizuje zapis liczb.
Prywatny workspace playlist.csv i wyjścia narzędzia hostowego nietknięte.

ID v1 nadal zależy od nazwy/URL/wystąpienia. Dodanie/usunięcie innej pozycji
nie przerywa sesji niezmienionego ID; usunięcie aktywnej pozycji zatrzymuje.
Zmiana nazwy/URL aktywnej stacji może zatrzymać ją (UI uprzedza). Trwałe ID
niezależne od edycji nadal OPEN, nie udajemy zrealizowania tego wymagania.

## Nowe API

- GET /api/v1/stations: generation string i stations[{id,name,url,ovol}],
  chunkowane JSON z buforem16KiB PSRAM; brak wielkiego cJSON całej listy.
- GET /api/v1/stations/export: attachment playlist.csv, chunkowane tekstowe TAB.
- POST /api/v1/stations/replace: text/plain, max256KiB, header
  X-StreamOO-Catalog-Generation,0B do jawnego wyczyszczenia,200 po zapisie.
- POST /api/v1/stations/select: JSON {"station_id":"000000000000000b"},
  ten sam generation header, max64B,202 do kolejki. Stale/nieistniejące409.

Nowe cztery endpointy tylko STA/AP-off, także po bezpośrednim wywołaniu
w AP409. Zapisy Origin/X-StreamOO-Request jak wcześniej, bez loginu.
Istniejący import nadal AP/LAN. Max URI handlers16.
Przeglądarka zachowuje draft przy konflikcie; zmiana trybu/nieaktualny GET
nie nadpisują nowych zmian. Podczas zapisu pola edytora są zablokowane.

## Weryfikacja

334 testy hostowe PASS (rozszerzone istniejące scenariusze). HTTP: lista/
eksport/select/clear, ID/generation, stale409, blokada nowych GET/POST w AP.
Store: CAS nie rusza poprzedniej listy, poprawna wymiana, trwały pusty
katalog i odtworzenie po ponownym init. Player/metadata dotychczas PASS.
Store/actor ASan/UBSan PASS (LSan off, bez TSan).
Chromium390px z syntetycznymi stacjami: AP setup-only, LAN defaultplayer,
kliknięcie dwóch stacji, volume/STOP/PLAY, add/edit/delete/export/import/
clear, konflikt zachowuje draft, anulowanie importu przy dirty, powrót AP
i brak overflow PASS. Podglądy player/editor sprawdzone na danych syntetycznych.
Native ESP-IDF6.1 build PASS,0x165fa0 (5% wolne), node/config/diff PASS.
Bez nowych bibliotek zewnętrznych, zmiany partycji/audio/DSP/ISR oraz
flash/erase/eFuse. Odbiór rzeczywistego urządzenia WEB_LIBRARY_BRINGUP.

Dodatkowo rzeczywiste nowe handlery portal configuration przeszły ASan/UBSan
(LSan off), obejmując listę/eksport/select/replace i blokady AP.
