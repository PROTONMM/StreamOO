# ADR 0017 — preferencje UI: zapis NVS po 5 sekundach

Data: 2026-10-24. Status: przyjęty; odbiór NVS na sprzęcie OPEN.

Pozostały zakres etapu 05 obejmuje trwałość volume/widżetu. Wprowadzamy
niezależny komponent preferences z czystą polityką i adapterem NVS/FreeRTOS.
Na razie zapisuje tylko volume 0..100 i visualization 0..2. Testowe ID stacji
oraz PLAY/STOP nie są trwałym stanem realnego playera i nie są zapisywane.
Nie dodajemy biblioteki: nvs_flash, esp_timer i FreeRTOS z ESP-IDF 6.1.
Sprawdzono lokalne publiczne nvs.h/nvs_flash.h, implementację nvs_api.cpp
oraz istniejącą partycję nvs 0x6000. nvs_flash ma licencję Apache-2.0;
nie skopiowano implementacji SDK do własnych źródeł.

Startup app_main inicjuje NVS, otwiera namespace streamoo i czyta ui_prefs
przed uruchomieniem UI. Brak klucza daje 50/spectrum i nie tworzy rekordu
preferencji. Sama inicjalizacja/otwarcie namespace może wykonywać operacje
wewnętrzne NVS. Nie używamy nvs_flash_erase ani automatycznego formatowania.
Błąd inicjalizacji, typu, długości, wersji lub zawartości pozostawia istniejące
dane; serwis nie startuje, aplikacja działa na domyślnych bez zapisu i loguje błąd.

Format blob: 8 bajtów, 'S','O',wersja=1,volume,visualization,0,0,0.
Walidujemy długość, magic, wersję, zakresy i zarezerwowane bajty. Nie zapisujemy
surowej struktury C. Integralność nośnika należy do CRC/mechanizmu NVS;
format nie jest poświadczeniem autentyczności ani szyfrowaniem.

Po starcie tylko zadanie preferences posiada uchwyt i wykonuje set_blob/commit.
UI przekazuje po każdym obsłużonym geście pełne wartości do nieblokującej
skrzynki długości 1 (latest wins). Jeden producent porównuje z ostatnio
przekazanym stanem: identyczne wartości nie resetują timera. Każda rzeczywista
zmiana niesie czas monotoniczny. Konsument resetuje deadline nawet gdy widzi
znowu tę samą wartość — skrzynka mogła skleić A→B→A. Nie wolno filtrować
wyłącznie końcowej wartości całej klatki, bo zgubiłoby to taki reset timera.

Po minimum 5s od ostatniej odebranej zmiany i tylko przy różnicy względem
potwierdzonego trwałego stanu zadanie zapisuje oba pola jednym blobem i commit.
Udany commit aktualizuje saved, błąd zachowuje dirty i odsuwa próbę o 5s.
Powrót do saved anuluje zbędny zapis. Aktualizacja podczas trwającego zapisu
pozostaje w skrzynce do kolejnej iteracji i nowego debounce; nie obiecujemy
anulowania operacji flash już rozpoczętej. Zapis i logi są poza rendererem
oraz przyszłą ścieżką I2S. Nie zmierzono jeszcze wpływu flash/cache na latencję.

Po restarcie aplikacyjny encoder_demo stosuje odczytane volume/widżet;
realne audio nadal nie jest uruchomione. Nagły reset/zanik przed zapisem może
utracić ostatnie zmiany. API kontrolowanego shutdown/flush pozostaje do
integracji z przyszłym właścicielem cyklu życia — dziś nie ma takiej komendy.
Nie uznajemy pełnego etapu 05 ani storage/security za ukończony.
