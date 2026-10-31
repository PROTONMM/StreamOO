# ADR0033 — zapisany katalog stacji na OLED

Data: 2026-10-31. Status: implementacja/build/testy hostowe PASS;
odbiór nowej listy na OLED OPEN. Użytkownik potwierdził nowy ekran z ADR0032.

## Mały krok przed readerem HTTP

Katalog wgrany przez istniejące WWW i zapisany w LittleFS jest źródłem listy
OLED w profilu sieciowym. Odczyt przy starcie i po udanym commit, bez restartu
po imporcie. Koniec syntetycznych nazw w tym profilu; pusty katalog daje
„Brak stacji” i wskazanie wgrania playlisty. Wybór enkoderem pokazuje nazwę
na HOME i zachowuje IP w trzeciej linii. Nie rozpoczyna jeszcze odtwarzania:
tytuł mówi „Odtwarzanie niedostępne”, widget nie pokazuje fikcyjnego pomiaru.
Samodzielny profil testów audio bez boot AP zachowuje swoje stacje/klipy TEST.

Nowy własny komponent stations: czysta projekcja kanonicznego CSV do
{name, URL, ovol, stable_id}. Bez alokacji/I/O/RTOS/zależności zewnętrznych.
Wstępna walidacja UTF-8/URL/limitów pozostaje w configuration. Pola name/URL
są zakończone NUL i wskazują bufor należący do snapshotu. ovol zachowane signed32,
bez automatycznego zastosowania do gain.

## Życie katalogu i atomowość

configuration tworzy niezmienny snapshot w PSRAM przed zapisem na flash.
Po udanym fsync/rename publikuje go pod krótkim portMUX; stary owner zwalnia
wyłącznie swoją referencję. Błąd alokacji/walidacji/zapisu nie publikuje nowej
wersji i zachowuje stary plik. Preview nie buduje/publikuje katalogu.

acquire/release O(1), bez NVS/LittleFS i oczekiwania na mutex trzymany podczas
zapisu. Widok ważny do release. Przy release ostatniej referencji zwolnienie
pamięci następuje poza sekcją krytyczną. Metadane snapshotu po publikacji
nie są modyfikowane. Czytelnik może utrzymać poprzednią wersję przez cały import.

UI raz na sekundę sprawdza referencję. Przy zmianie tworzy projekcję wpisów
w PSRAM; dopiero po udanej podmianie zwalnia stary widok/nazwy. HOME czyści
stare borrowed pointers, lista jest zamykana bez komendy wyboru. Brak pamięci
odracza odświeżenie, zamiast kasować dotychczasową listę. Klatki renderowania
nie czytają flash ani nie alokują katalogu. Brak zmian w ścieżce I2S.

Zachowujemy wybór, gdy ID nadal istnieje; usunięcie wybranej pozycji czyści
wybór/stan lokalny, bez automatycznego wskazania innej stacji. Przy pierwszym
odczycie nie wybieramy losowo stacji; wybór należy do użytkownika. Zapis
ostatniego wybranego ID w NVS jest kolejnym zakresem, nie częścią tego kroku.

## Identyfikatory v1

Deterministyczny FNV-1a 64-bit: URL, separator NUL, nazwa, separator NUL,
32-bit little-endian numer wystąpienia dla powtarzających się par name/URL.
ovol nie wpływa na ID. Unikalne pary mają ten sam ID po przestawieniu listy,
reboocie i zmianie ovol. Duplikaty zachowane z różnymi ID, zero zarezerwowane;
wykryta kolizja odrzuca cały kandydat przed zmianą pliku.

Ograniczenie: zmiana nazwy/URL oznacza nową tożsamość w tym etapie. W obrębie
powtarzających się identycznych par name/URL numer wystąpienia rozróżnia kopie,
nie historię ich ręcznych edycji. Przed edytorem stacji potrzebna polityka
trwałych ID/mapowania przy rename/URL change i migracji algorytmu. Nie uznajemy
pełnego modelu zarządzania stacjami z etapu08 za zakończony.

## Weryfikacja

226 testów hostowych PASS. Nowe testy modelu: reorder, zmiana ovol, polskie
znaki, duplikaty, signed32 i limity. Test store: czytelnik utrzymuje stary
snapshot po commit, nowa generacja, ten sam ID po odczycie z pliku; preview,
błąd rename i awarie alokacji header/bufora/tabeli nie zmieniają starego katalogu.
Test UI: prawdziwa lista, wybór nazwy, reorder zachowuje wybór, zamknięcie listy,
czyszczenie starych pointerów i usunięcie wyboru bez fallback.
Testy store i controls również PASS z ASan/UBSan, detect_leaks=0; fake portMUX
na hoście nie dowodzi braku wyścigów na sprzęcie, TSan nie uruchamiano.
Prywatny CSV wyłącznie w RAM: 267 rekordów, 267 unikalnych ID, te same ID po
odwróceniu kolejności; oryginał niezmieniony i nieśledzony. Brak kopii danych
w repo, fixture, obrazie ani nazw/URL/ID w logu. Native ESP-IDF6.1 build PASS.

HARDWARE_TEST_PENDING: lista z wcześniej zapisanej playlisty po starcie,
polskie nazwy/scroll/wybór/IP, drugi import podczas otwartej listy, reboot
katalogu, heap/stosy. Nie wykonano flash/erase/eFuse. Audio sieciowe nadal OPEN.
