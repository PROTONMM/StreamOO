Nowszy ADR0051 / WEB_LIBRARY_BRINGUP: w AP wyłącznie konfiguracja,
po STA/AP-off domyślnie Odtwarzacz z listą stacji. Wybór przez WWW już
działa; poniższe wymaganie wyboru enkoderem dotyczy poprzedniego zakresu.

# Sterowanie WWW — pierwszy odbiór

1. Zwykły flash bez erase/NVS/obrazu FS. WWW odśwież Ctrl+F5, otwórz Status.
2. Po STA wybierz działającą stację enkoderem. Sprawdź Odtwarzaj/Zatrzymaj
   w WWW i zmianę stanu/dźwięku. PLAY używa wybranej stacji; bez wyboru
   dostaniesz komunikat o niedostępnym odtwarzaczu. Lista WWW później.
3. Ustaw volume suwakiem. OLED ma pokazać to samo, audio korzysta z
   istniejącej rampy. Obróć enkoder: WWW ma odczytać zmienioną wartość
   w kolejnym statusie (około2s). Powtórz zmiany z obu źródeł.
4. STOP/PLAY kilka razy, także ponowny PLAY podczas grania: nie powinien
   restartować aktywnej sesji. Przy retry PLAY ponawia tę samą stację.
5. AP/offline: sterowanie niedostępne, nie startuje fixture/audio.

Odpowiedź „Polecenie przyjęte” oznacza kolejkę; faktyczny stan potwierdza
status. Sterowanie ma pozostać płynne, bez trzasków. Test NVS strefy po
power-cycle użytkownik wykona niezależnie, bez blokowania tego odbioru.
