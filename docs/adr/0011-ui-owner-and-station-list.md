# ADR 0011 — właściciel stanu UI i animowana lista

Data: 2026-10-24. Status: część programowa 04h przyjęta,
odbiór nowej listy na OLED HARDWARE_TEST_PENDING.

## Zakres i zależności

Nowy oryginalny komponent C11 `ui` zależy od graphics i text, bez GPIO,
SSD1322, playera, NVS, sieci i RTOS. Sprawdzono dostępność jego API przez
hostowy kompilator z -Werror i build ESP-IDF 6.1. Brak nowych bibliotek
zewnętrznych i importu kodu z yoRadio; istniejące fonty zachowują wcześniejsze
noty licencyjne. To właściciel ekranu HOME/STATIONS i wyboru na liście,
nie implementacja całego przyszłego player/network/UI managera.

Obiekt UI ma jednego właściciela zadaniowego. Przyszły adapter wejścia
przekaże mu zdarzenia z kolejki; callbacki HTTP/GPIO nie mogą mutować go
równolegle. API otwiera listę, przesuwa kursor, anuluje lub zatwierdza wybór.
Nie ma globalnego stanu, alokacji, I/O ani oczekiwania na mutex.

## Model katalogu i polecenia

UI pożycza immutable snapshot {stable_id, name, name_bytes} i generation.
Repository stacji odpowiada za poprawność/unikalność identyfikatorów oraz
życie tablicy i napisów do kolejnego set_catalog. UI sprawdza podstawowe
pola i wskaźniki; nie jest parserem uploadowanego katalogu. stable_id jest
opaque uint64 tokenem; nie jest indeksem listy ani ustaleniem formatu importu.

playing_id jest jedynie otrzymanym snapshotem, aktualizowanym z zewnątrz.
Otwieranie wybiera go na liście, a jeśli go nie ma — pierwszy wpis.
Nawigacja i anulowanie nie wysyłają żadnego polecenia. Confirm przekazuje
{id, catalog_generation} do nonblocking adaptera kolejki, który musi
skopiować dane. UI nigdy nie zmienia playing_id po confirm. Odtwarzacz
w późniejszym etapie sprawdzi aktualność polecenia i zaktualizuje snapshot.
Nie deklarujemy już wdrożonego player generation guard ani ciągłości audio.

Po przyjęciu polecenia UI wraca do HOME natychmiast. Przy odmowie (np. pełna
kolejka) pozostaje na liście, zachowując wybór do ponowienia. Anulowanie
zamyka listę bez select. Wymiana katalogu zamyka przeglądanie, nie emituje
polecenia i pozwala właścicielowi zwolnić poprzedni snapshot. Ponowne otwarcie
szuka playing_id w nowej kolejności. Identyfikator nie zależy od indeksu.

## Animacja i decyzje otwarte

Logiczny selected jest oddzielony od pozycji animowanej. Pozycja używa
Q16 zamiast float; czas interpolacji jest argumentem init (proponowane
160ms, 0 wyłącza animację). Retarget zaczyna od aktualnej pozycji, więc
szybkie detenty nie powodują skoku ani utraty ostatniego celu. Confirm
w trakcie animacji zatwierdza ostatni logiczny, wyróżniony cel. Po uspokojeniu
jest on dokładnie środkowym wpisem. Pozycje wszystkich wierszy liczone są
ze wspólnego przesunięcia pikselowego, zachowując odstęp 16px podczas ruchu.

Wybrany wpis jest jaśniejszy i używa StreamOO UI16; sąsiednie wpisy są
przygaszone i mają mały font. Długi wyróżniony napis korzysta z lewostronnego
marquee. Pusty katalog ma komunikat „Brak stacji”. Clipping izoluje listę.

D19 pozostaje OPEN: w wariancie testowym granice zatrzymują kursor, bez
wrap-around; napisano test tego zachowania. Nie uznano tego za zatwierdzony
docelowy UX. Nie dodano samoczynnego timeoutu zamykania listy.

## Diagnostyka i weryfikacja

Po dotychczasowych 38s testu dodano 8s pokazu listy TEST: otwarcie,
przewijanie, granica, potwierdzenie i ponowne otwarcie. Katalog i zdarzenia
są syntetyczne. Odbiorca komend jest jawnym sinkiem diagnostycznym, nie
playerem; zapamiętuje wybór 1003, pozostawiając snapshot 1002. Log to pokazuje.
Cały test trwa około 46s i kończy się OFF. Enkoder nie steruje jeszcze listą.

11 nowych testów obejmuje wybór stable_id/generation, brak komend przy
browse/cancel, odmowę kolejki i retry, retarget, duże delty, wymianę katalogu,
pustą listę, clipping, jasność, animację 0ms i pełny skrypt diagnostyczny.
Scenę referencyjną obejrzano na hoście przed zamrożeniem. ASan/UBSan sprawdza
skrypt i 1000 retargetów; LeakSanitizer jest niedostępny pod ptrace, użyto
wyłącznie znanego trybu detect_leaks=0. Nie raportujemy testu wycieków.

Odbiór OLED obejmuje płynność i czytelność listy. Fizyczne gesty, player,
repozytorium stacji i pełny ekran HOME są w dalszych etapach. Własność UI
nie oznacza zaimplementowania tych komponentów przez sam model testowy.
