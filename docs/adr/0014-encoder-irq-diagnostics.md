# ADR 0014 — przechwytywanie zboczy enkodera

Data: 2026-10-24. Status: przyjęty; test sprzętowy OPEN.

Użytkownik zgłosił, że obracanie w obu kierunkach zmienia A, natomiast
B pozostaje 0 i licznik sampled edges B wynosi 0. Potwierdził enkoder
półkrokowy; istniejące ustawienie HALF_QUADRATURE=y pozostaje bez zmian.
Próbkowanie co 10ms nie rozstrzyga, czy B nie dostarcza sygnału, czy
pominięto jego impulsy. Nie przypisujemy przyczyny bez eksperymentu.

Adapter hardware_probe zastępuje polling wejść przerwaniami ANYEDGE dla
A/B/BTN. ISR odczytuje poziomy i czas esp_timer, inkrementuje osobne liczniki
wywołań i przekazuje próbkę do kolejki 128 elementów. Nie loguje, nie alokuje,
nie dekoduje i nie steruje UI. Zadanie o priorytecie 3 przetwarza kolejność
próbek, a po timeout przesuwa debounce na ostatnich uchwyconych poziomach.
Monotoniczny czas dekodera jest zachowany przy wyścigu timeout/ISR.

Przepełnienie jest jawne: utracone próbki są zliczane, zadanie zatrzymuje
przerwania, odrzuca kolejkę, inicjuje dekoder z aktualnych poziomów i blokuje
klik aż do kolejnego zaakceptowanego zwolnienia. Próbę należy wtedy powtórzyć.
Liczniki IRQ obejmują także odbicia i nie są liczbą ząbków. Odczyty 3 pinów
nie są równoczesnym zatrzaskiem; GPIO może scalać bardzo szybkie zbocza.
ISR bez flagi IRAM nie daje gwarancji obsługi podczas wyłączenia cache.
Nie deklarujemy bezstratności ani zmierzonej maksymalnej szybkości.

API sprawdzono w lokalnym ESP-IDF 6.1: esp_driver_gpio gpio.h/gpio.c,
esp_timer oraz FreeRTOS. Brak nowej zależności lub kopiowania implementacji.
GPIO 5/16/7 i pull-up pozostają zgodne z konfiguracją, nie zmieniamy
mapowania na podstawie samego logu. Start wymaga własnej usługi GPIO ISR;
zajęta usługa i błędy startu są zwracane, zasoby po błędzie zwalniane.

Kolejny test: IRQ edges A i B muszą rosnąć podczas obrotu; przeciwne
kierunki powinny dawać przeciwne delta. Jeżeli A rośnie, B=0 także w IRQ,
należy sprawdzić tor B do skonfigurowanego GPIO16 (połączenie, styk,
podciąganie). Stały poziom spoczynkowy sam w sobie nie identyfikuje usterki.
