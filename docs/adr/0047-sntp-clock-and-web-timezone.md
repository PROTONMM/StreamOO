# ADR0047 — zegar SNTP i strefa wybierana przez WWW

Data:2026-10-31. Status: wdrożone, odbiór sprzętowy OPEN.

## Decyzja użytkownika

Strefa ma być wybierana w zakładce Konfiguracja WWW, z automatycznym
czasem letnim/zimowym. To zastępuje propozycję wyboru wyłącznie przy
kompilacji. Domyślnie Europe/Warsaw, ustawienie trwałe, działa bez restartu.
Układ OLED i format YYYY-MM-DD / HH:MM:SS pozostają jak dotychczas.

## Źródło i prezentacja czasu

Istniejący SNTP pool.ntp.org uruchamiany po STA. Callback sync_cb publikuje
ważność dopiero po synchronizacji i dla UTC2020..2099. Wcześniej OLED
pokazuje kreski, niezależnie od tego, co zwraca niezainicjowany system time.
Po pierwszym sync zegar biegnie również bez Wi-Fi, do restartu; brak RTC
bateryjnego i dokładność dryfu nie są tu testowane. Nieważna kolejna
korekta SNTP unieważnia prezentację. Utrata STA sama nie kasuje synchronizacji.

Własny clock_project jest czysty: int64 UTC → czas cywilny, bez network,
libc TZ, globalnego setenv/tzset, alokacji i I/O. UTC pozostaje UTC dla TLS.
Adapter display odczytuje time(NULL), cached zone i network.time_synced;
renderer tylko rysuje snapshot. Clock działa także bez wybranej stacji/STOP,
zmiana sekundy nie resetuje marquee. Compile-time assert wymaga64bit time_t.

Pierwszy katalog8 stref: UTC, Europe/Warsaw, Europe/London, Europe/Berlin,
Europe/Paris, Europe/Helsinki, Europe/Madrid, Europe/Rome. To jawna lista,
nie pełna baza IANA. Europejskie przejścia według obecnej reguły: ostatnie
niedziele marca/października,01:00UTC. UTC bez DST, Londyn0/+1, Helsinki
+2/+3, pozostałe+1/+2. Reguły nie aktualizują się z internetu; zmiana prawa
lub rozszerzenie katalogu wymaga aktualizacji firmware.

Źródła sprawdzenia reguł2026-10-31:
- https://transport.ec.europa.eu/transport-themes/summertime_en
- https://eur-lex.europa.eu/legal-content/EN/TXT/?uri=CELEX:52021XC0427(01)
API sync_cb i64bit time_t zweryfikowane w przypiętym ESP-IDF6.1.
Brak nowych zależności zewnętrznych i importu kodu stron trzecich.

## Trwałość i WWW

Clock settings osobny owner: NVS namespace streamoo_clock/key zone, string
z listy, odczyt raz przy starcie, cache chroniony krótką sekcją krytyczną.
HTTP zapis serializowany mutexem, update cache dopiero po udanym commit.
Ponowny zapis już utrwalonej tej samej strefy nie zapisuje flash. Brak
klucza/nieznana wartość → domyślna Warszawa bez kasowania NVS; błąd
odczytu/init wyłącza zapis, zachowuje default i nie zatrzymuje urządzenia.

GET /api/v1/clock zwraca timezone i listę zones. POST przyjmuje wyłącznie
obiekt {"timezone":"Europe/Warsaw"}, max128B, JSON, exact whitelist,
Origin i X-StreamOO-Request jak reszta konfiguracji. Bez osobnego loginu.
Nieudany commit zwraca503 i nie zmienia aktywnej strefy. Max URI handlers11.
Frontend ma zakładki Status/Konfiguracja, domyślnie Konfiguracja; w niej
istniejące Wi-Fi/import oraz Data i czas. Select z listą API, zapis od razu
i zachowany po restarcie. Błąd informowany w formularzu, bez zmiany playera.

## Sprawdzenie

328 testów hostowych PASS.5 testów konwersji: porównanie Python zoneinfo
co tydzień2020..2099 dla8 stref oraz dokładne sekundy wokół obu przejść
DST w każdym roku, leap day/year rollover/błędy.6 testów rzeczywistego
settings ownera z NVS fake: restore, default, brak powtórnych zapisów,
commit/read/init/mutex failure i nieznana strefa. Actor sieci: zły czas
SNTP nie daje ważności, poprawny callback ją ustawia. UI: zegar bez stacji,
bez resetu scroll i placeholder. App startup zachowuje audio przy awarii
ustawień. HTTP: GET/POST, whitelist, Origin/header/body limit, błąd zapisu.
Clock-settings/network/controls ASan/UBSan PASS (LSan off, bez TSan).

Chromium390px: tabs, odczyt/zapis strefy, brak overflow, dotychczasowe
Wi-Fi i import PASS z syntetycznym API. Node --check PASS.
Native ESP-IDF6.1 build PASS,0x160ff0 (6% wolne); GPIO validator/diff PASS.
Pcm_fill w IRAM; audio/DSP/ISR i partycje bez zmian. Bez flash/erase/eFuse.
Rzeczywiste NTP, zapis/reboot i zegar OLED OPEN: CLOCK_BRINGUP.
