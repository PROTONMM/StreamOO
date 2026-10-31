# ADR0050 — podstawowe sterowanie playerem przez WWW

Data:2026-10-31. Status: wdrożone, odbiór sprzętowy OPEN.

Użytkownik potwierdza działanie bieżącego firmware i kontynuuje. Fizyczny
test trwałości strefy wykonuje osobno — nie uznajemy go za zaliczony.
Kolejny zakres roadmapy: lokalne sterowanie WWW jako podstawa przyszłej
aplikacji telefonu, bez drugiego ownera odtwarzania.

## Polecenia i API

POST /api/v1/player/control przyjmuje JSON:
- {"action":"play"}
- {"action":"stop"}
- {"action":"volume","value":57}

Dokładna liczba pól, whitelist akcji, volume integer0..100, body<=128B,
Origin i X-StreamOO-Request jak pozostałe lokalne zapisy; bez dodatkowego
logowania.202 oznacza skopiowanie do kolejki, nie zakończenie połączenia
ani wymuszone PLAYING. Stan nadal GET /api/v1/status.400 dla złych danych,
409 dla braku dostępnego playera/wybranej stacji przy PLAY,503 dla kolejki.

Main relay odrzuca akcje przy braku STA i aktywnym AP. PLAY używa wyłącznie
wybranej stacji, bez wyboru zastępczej. W tym kroku lista/wybór przez WWW
jeszcze OPEN; najpierw wybór enkoderem. Głośność można ustawić w LAN
również przy STOP, bez uruchamiania źródła.

Player ma nowe task-only API set_playing/set_volume. Oba kopiują polecenia
do tej samej kolejki co encoder (brak I/O/alloc/wait). Explicit PLAY/STOP
są idempotentne, nie używają toggle. PLAY przy trwającym retry resetuje
backoff tej samej stacji, nie zamienia go na STOP ani nie ignoruje błędu
I2S w aktywnej sesji. SET_VOLUME to wartość absolutna, nie delta wyliczona
ze starego snapshotu. Encoder delta i WWW SET są wykonywane kolejno przez
jednego ownera; ostatnie polecenie wygrywa. Revision/ramp i zapis volume
po5s bez zmian. STOP zachowuje istniejące fade/reader ACK/cleanup.

## WWW

Zakładka Status: Odtwarzaj/Zatrzymaj i suwak0..100. Input aktualizuje tylko
podgląd wartości, change wysyła jedno polecenie. Odpowiedź202 nie ustawia
optymistycznie running; UI czeka na status playera. Poll nie nadpisuje
wartości podczas przeciągania/POST, generation guard odrzuca odpowiedź
statusu sprzed polecenia. W trakcie POST i przy AP/offline elementy disabled.
Konfiguracja sieci/stacji/zegara bez zmiany kontraktu. Max URI handlers12.

## Weryfikacja

334 testy hostowe PASS. Actor: idempotentne STOP/PLAY, absolute volume
razem z encoder delta, wartości0/100, reject101, brak revision przy tym
samym volume i PLAY podczas retry bez toggle. HTTP: walidacja formatu/
zakresu/pól, proof, kody202/409/503; istniejące konfiguracje PASS.
Actor ASan/UBSan PASS (LSan off, bez TSan).
Chromium390px z syntetycznym API: AP controls disabled, LAN → suwak63,
STOP i PLAY potwierdzone kolejnymi statusami, brak overflow; dotychczasowe
Wi-Fi/import/timezone nadal PASS. Node --check PASS.
Native ESP-IDF6.1 build PASS,0x161f00 (6% wolne); validator/diff PASS.
Bez nowych zależności, zmian DSP/ISR/partycji i flash/erase/eFuse.
Sprzęt: WEB_PLAYER_BRINGUP.
