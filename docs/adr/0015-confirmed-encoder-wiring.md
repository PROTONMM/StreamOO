# ADR 0015 — potwierdzone połączenie enkodera 5/6/7

Data: 2026-10-24. Status: przyjęty na podstawie informacji użytkownika.

Schemat referencyjny prowadzi S1E2 do IO16, ale użytkownik stwierdził,
że używany enkoder jest fizycznie podłączony do GPIO5/6/7. Po zmianie B na 6
log wykazuje IRQ obu kanałów oraz ROTATE o przeciwnych delta, SHORT, LONG
i PRESSED_ROTATE. Przyczyną braku B było niewłaściwe dla sprzętu przypisanie.
To jawny wyjątek od mapy schematu, nie wnioskowanie z dawnych definicji Arduino.

Aktualizujemy domyślny B=6 w Kconfig i sdkconfig.defaults, dokumentację
sprzętu i główny prompt. Nie zmieniamy schematu źródłowego ani historii ADR.
A=5, BTN=7 i półkrok pozostają bez zmian. Nie włączamy dodatkowego enkodera;
GPIO6 używane w jego referencyjnej mapie jest zajęte przez ten kanał B.

Log nie identyfikuje obrotu zgodnego z zegarem ani liczby zdarzeń na ząbek.
Nie potwierdza również ciągłego trzymania przycisku po PRESSED_ROTATE.
Te kwestie i pełny odbiór gestów pozostają otwarte. Zmiana dotyczy mapowania;
nie wymaga nowej biblioteki ani zmiany dekodera.

## Doprecyzowanie kierunku po teście interaktywnym

Użytkownik potwierdził działanie 05c, ale obrót w lewo zwiększał volume,
a w prawo zmniejszał; lista miała ten sam odwrócony kierunek.
Ustawiamy domyślne ENCODER_REVERSED=y w Kconfig i sdkconfig.defaults.
Zmiana znaku pozostaje w dekoderze, wspólna dla zwykłego i wciśniętego
obrotu; nie odwracamy osobno adaptera listy ani volume. Oczekiwane:
prawo zwiększa volume i indeks stacji, lewo zmniejsza. Piny bez zmian.
