# ADR 0012 — pełny renderer HOME i snapshot danych

Data: 2026-10-24. Status: 04i programowo zweryfikowane;
nowa kompozycja HOME HARDWARE_TEST_PENDING.

## Zakres i odpowiedzialność

`ui` otrzymało model HOME i renderer w układzie 256×64:
- y0..15: centrowana nazwa, StreamOO UI16 na bazie Krypton;
- y16..23: tytuł, y24..31: opis, mały font PL i lewostronny marquee;
- dół 44/132/80: codec/bitrate/Wi-Fi, wybrany widżet, data/czas.

Szerokości dolnych paneli pozostają proponowane. Kod jest oryginalnym C11,
korzysta z istniejących graphics/text/ui_widgets, bez nowych bibliotek.
Dostępność API i typów sprawdzono kompilacją hostową i ESP-IDF 6.1.
Bez sterownika ekranu, I/O, czasu systemowego, Wi-Fi, FFT i alokacji w rendererze.
Renderer nie uruchamia usług sieciowych ani odtwarzania przez samo wyświetlenie.

Właściciel UI przechowuje kopię wartości snapshotu i pożyczone wskaźniki do
napisów. Producent utrzymuje napisy do następnego update. Snapshot jest pełny,
nie jest patchem samej sieci/czasu; przyszły agregator łączy źródła z aktualnym
kluczem playera. Sieć i poprawny czas mogą być niezależne (np. zegar działa
bez Wi-Fi). UI nie wybiera strefy ani nie synchronizuje czasu.

## Identyfikacja, metadane i stan

station_id/stream_generation identyfikują bieżący strumień. Generacja rośnie
przy zmianie strumienia/stacji. data_station_id/data_generation tagują wszystkie
dane pochodzące ze strumienia: metadane, info audio i pomiary. Niezgodne tagi
wyłączają grupy, usuwają pożyczone wskaźniki i zerują pomiary. Nie odczytujemy
nieaktualnych tytułów/codec nawet gdy producent pozostawił stare dane.

Cały starszy snapshot jest odrzucany. W tej samej generacji odrzucana jest
starsza text_revision lub inna stacja. text_revision rośnie przy zmianie
napisów. Klucz, rewizja lub widoczność metadanych resetuje czas marquee;
aktualizacja sekund zegara albo RSSI go nie resetuje. Po samym set_playing
stare metadane są natychmiast czyszczone, a następny pełny snapshot musi
mieć nową generację. playing_id pozostaje snapshotem playera, nie jest
zmieniany przez confirm na liście. Aktualizacja HOME nie rusza kursora listy.

Wybór widżetu jest stanem UI (set_visualization), nie stanem DSP. Rysowanie
HOME jest const względem UI. Dane aktywne są walidowane przed atomowym
zastąpieniem cache; niepoprawny snapshot pozostawia poprzedni zaakceptowany.
Model i deskryptor canvas są zaufanymi danymi aplikacji, nie parserem danych HTTP.

## Brakujące dane i formaty do dostrojenia

Bez stacji: „Brak stacji”. Bez nazwy: nazwa z katalogu lub „Brak nazwy”.
Brak metadanych czyści dwa wiersze; brak codec/bitrate daje kreski, brak
pomiarów „Brak danych”, a nie wykres udający ciszę. Przy nieznanym czasie
widoczne są kreski, bez zmyślonej daty.

Format diagnostyczny: YYYY-MM-DD i HH:MM:SS; D20 pozostaje OPEN. Producent
podaje gotowy czas cywilny. Walidacja obejmuje dni miesiąca i lata przestępne,
rok 1..9999 oraz czas 0..23/0..59/0..59. Nie wdrożono polityki SNTP/RTC.

Wi-Fi: cztery słupki, OFF przy rozłączeniu, ? bez RSSI. Proponowane progi
RSSI: -80/-70/-60 dBm; to geometria wskaźnika do strojenia, nie pomiar jakości
sieci. Dokładne RSSI jest obok. Bitrate jest kompaktowe: k, M lub G, przy
większych liczbach część ułamkowa jest obcinana; 0 oznacza brak danych.

## Diagnostyka i odbiór

Dawne osobne sceny text_layout_smoke/visualization_probe zastąpił home_probe:
0..12s komplet jawnie syntetycznych danych (TEST), 12..15s nowa stacja ze
starymi tagami metadanych, 15..18s poprawne nowe dane, 18..20s brak stacji,
Wi-Fi i czasu. TEST w prawym dolnym wierszu jest nakładką diagnostyki,
nie elementem produkcyjnego renderera. Widżet zmienia się co 4s.
Cała sekwencja nadal około 46s, z niezmienionym 8s pokazem listy po HOME.
Nie dołączono prawdziwego playera, sieci, SNTP ani DSP.

Usunięto nieużywane sceny aplikacyjne i ich dawny wzorzec; zastąpiły je cztery
przejrzane klatki HOME. Testy pokrywają stare snapshoty, znikanie metadanych,
clock/RSSI, kalendarz, reset scrolla, clip, widżety i brak poleceń do playera.
ASan/UBSan obejmuje cały pokaz i dotychczasową listę z nowym modelem.
LeakSanitizer nadal niedostępny pod ptrace: detect_leaks=0, bez testu wycieków.

Następna brama: czytelność HOME, ikonki i zegara na OLED oraz brak starych
tekstów w przejściu. Fizyczne sterowanie i gesty nadal etap 05. Renderer
nie oznacza ukończenia rzeczywistych funkcji radia ani finalnej typografii.
