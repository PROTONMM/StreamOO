# ADR0082 — rozdzielenie FPS UI od skanowania OLED

Data: 2026-11-01. Status: analiza kodu/dokumentacji; przyczyna sprzętowa OPEN.

Użytkownik zgłosił pulsowanie i poziome prążki na bargraph oraz spectrum
i zapytał o nadmierne odświeżanie. UI wysyła pełną klatkę co 40 ms,
bez synchronizacji ze skanowaniem; payload trwa co najmniej 6,5536 ms
przy 10 MHz. Brak osobnego wysyłania czerni i wygaszania pomiędzy klatkami.

Decyzja: nie uznawać 25 FPS za potwierdzoną przyczynę i nie zmieniać
w ciemno profilu analogowego SSD1322. Najpierw porównać ruchome widżety
z logo/wzorcem bez transferów oraz obraz gołym okiem z obrazem kamery.
Dalszy eksperyment zmienia tylko jedną zmienną: FPS aplikacji lub B3.
Nie utożsamiać odświeżania RAM, PWM półtonów i skanowania panelu.

Źródła, obliczenia i procedura: [OLED_TIMING_BRINGUP](../OLED_TIMING_BRINGUP.md).
Zweryfikowano ścieżkę renderer→present→SPI i ustawienia B3/B1/B9.
Brak zmian kodu, nowych zależności i nowego buildu; dotychczasowe wyniki
testów/buildów pozostają historycznym wynikiem, nie testem tego objawu.
Agent nie wykonywał flash ani pomiaru fizycznego panelu.
