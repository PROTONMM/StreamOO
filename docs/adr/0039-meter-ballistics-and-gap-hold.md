# ADR0039 — spokojniejsze wskaźniki i krótkie luki pomiarów

Data: 2026-10-31. Status: przyjęte na prośbę użytkownika; odbiór OLED OPEN.

Użytkownik potwierdził działanie07b, ale wskaźniki są za szybkie i czasem
miga „Brak danych”. Log: PLAYING44100/PCM32, invalid/low/hard0, starts1,
max_fill133us. Jednocześnie gaps dochodzi do23, drop28, max_work14252us;
HWM analizatora minimum3280B, heap internal minimum35136B. Są luki tap,
nie dowód niedoboru audio. Poprzednio każda luka od razu unieważniała
poziomy UI, nawet jeśli poprzednie kompletne okno było świeże.

## Decyzja

Nie zmieniamy próbkowania audio ani tempa UI/enkodera. Własna prezentacja
ballistics w levels: aktualizacja poziomów najwyżej10Hz (100ms), RMS attack
160ms/release500ms, peak hold400ms i release800ms. Wygładzanie pierwszego
rzędu: alpha=dt/(tau+dt), obliczenia Q16, czas monotoniczny; nie jest to
kalibrowany VU. Szczyty ze wszystkich kompletnych okien między aktualizacjami
zachowane jako maksimum. Peak nie opada poniżej RMS. Start nowej sesji od0.
Obliczenia w istniejącym tasku analizy, bez nowych zadań i alokacji.

Rdzeń RMS dalej resetuje niepełne okno po luce. Prezentacja zachowuje
ostatni kompletny pomiar do dotychczasowego limitu200ms. Sama luka nie
odświeża timestampu; prawdziwy brak pomiarów nadal daje „Brak danych”.
Generation, STOP, retry, buffering i AP nadal czyszczą wynik od razu.
Nie ukrywamy liczników gaps/drop i nie interpolujemy brakujących próbek.

## Sprawdzenie

286 testów hostowych PASS, ballistics/metering ASan/UBSan PASS (LSan off).
Testy: takt100ms, asymetryczna reakcja, peak hold i opadanie, zachowanie
krótkiego szczytu między taktami, reset sesji, brak reakcji na nieważny
pomiar, krótka luka bez migania, expiry i faktyczny worker.
Brak nowych zależności/kodu stron trzecich, brak zmian ścieżki I2S.
Odbiór szybkości/wygładzania na OLED wymaga użytkownika.

Native ESP-IDF6.1 build PASS, obraz0x15bf20 (7% wolne);
check_config --platform i diff --check PASS. Bez flash/erase/eFuse.
