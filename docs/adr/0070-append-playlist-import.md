# ADR0070 — import playlisty dopisuje stacje

Data: 2026-10-31. Status: wdrożone do odbioru sprzętowego.

Użytkownik wymaga, aby wgranie playlist.csv przez WWW nie usuwało zapisanych
stacji. Usunięcie całej listy pozostaje świadomą, osobną akcją.

Preview importu nadal waliduje wyłącznie wskazany plik i pokazuje liczbę
importowanych pozycji. Commit ponownie normalizuje plik, odczytuje bieżący
niezmienny snapshot katalogu, zapisuje jego rekordy i rekordy importu do
jednego tymczasowego katalogu, waliduje łączny limit i atomowo podmienia plik
LittleFS. Publikacja używa generation guard: równoczesny zapis edytora daje
409 catalog_changed, nigdy utratę jego zmian.

Istniejące rekordy zachowują stable ID. Nowe rekordy dostają nowe niezerowe
ID; wpisy o takim samym URL są nadal dozwolone i raportowane jako duplikaty,
zgodnie z dotychczasową polityką. Edytor z zapisem katalogu nadal jawnie
zastępuje całą jego wersję, a clear jest jedyną operacją pustego katalogu.

Test magazynu obejmuje dopisanie 2 rekordów do katalogu z 1 rekordem,
zachowanie ID starej pozycji, podgląd bez zapisu oraz błędy alokacji/rename
bez zmiany aktywnego snapshotu. Test portalu obejmuje preview/commit i
frontend komunikaty nie mówią już o zastępowaniu. Odbiór: eksportuj listę
lub zapamiętaj jej liczbę, importuj mały plik i sprawdź sumę oraz stare
pozycje po reboocie. 381 testów hostowych oraz native ESP-IDF6.1 build
PASS0x17bf70 (90% app wolne); agent nie flashował.
