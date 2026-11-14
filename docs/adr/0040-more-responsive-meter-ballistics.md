# ADR0040 — mniejsze wygładzanie mierników

Data: 2026-10-31. Status: przyjęte na polecenie użytkownika, odbiór OLED OPEN.

Po4087b9b użytkownik: działa, ale mierniki ruszają się zbyt mało i dane
są zbyt uśrednione. Zmieniamy wyłącznie parametry prezentacji ADR0039:
RMS attack160→30ms, release500→150ms; peak hold400→150ms, release800→300ms.
Takt10Hz, okno RMS40ms, skala liniowa i podtrzymanie świeżego wyniku przez
krótką lukę pozostają. Brak nowych obliczeń w ISR, zadań, alokacji i zależności.

Dla skoku RMS0→20000 pierwszy takt100ms osiąga ponad75% celu zamiast38%.
Po spadku do0 po200ms pozostaje mniej niż40% poziomu. Szczyt zaczyna
opadać po150ms; krótkie szczyty między taktami nadal zachowane. Testy
sprawdzają te granice, zachowanie taktu i rzeczywisty konsument pomiarów.
To strojenie reakcji, bez zmiany głośności audio ani kalibracji VU/dB.

Weryfikacja:286 testów PASS, ballistics/metering ASan/UBSan PASS (LSan off).
Native ESP-IDF6.1 build PASS,0x15bf20 (7% wolne).
check_config --platform i diff --check PASS.
