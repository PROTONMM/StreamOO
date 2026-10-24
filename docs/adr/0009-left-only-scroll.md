# ADR 0009 — przewijanie wyłącznie w lewo

Data: 2026-10-24. Status: kierunek USTALONY przez użytkownika,
implementacja programowo zweryfikowana; nowy cykl HARDWARE_TEST_PENDING.
Zastępuje ruch ping-pong z ADR 0007.

## Potwierdzenie i wymaganie

Użytkownik potwierdził działanie ostatniego firmware, poprawność polskich
znaków i działanie scrolla. Obecna czcionka jest zaakceptowana na ten etap,
ale wymaga późniejszego dopracowania wyglądu. Nie traktujemy tego jako
zatwierdzenia docelowej typografii ani jako pomiarów CPU/FPS/SPI.
Nowe wymaganie: ruch wyłącznie od prawej do lewej, bez animacji powrotnej.

## Decyzja wykonawcza

Długie teksty tworzą cykliczny pas: tekst i proponowany odstęp 24px.
Rysowane są najwyżej dwie kopie tego samego źródła UTF-8, przez istniejący
clip. Następna kopia wchodzi z prawej, podczas gdy poprzednia wychodzi lewą
krawędzią. Nie zmieniamy źródła, nie dopisujemy spacji do bufora i nie
alokujemy pamięci. Krótki tekst nadal korzysta z left/center/right.

Pozostają parametry 30px/s i pauza 1000ms przy wyrównanym początku cyklu;
nie ma pauzy przy końcu tekstu ani cofania. Odstęp jest stałą
STREAMOO_MARQUEE_GAP_PX, parametry są do dostrojenia na OLED. Po zmianie
tekstu/stacji właściciel UI resetuje elapsed do 0, tak jak w ADR 0007.

Zegar i offset używają uint64, ponieważ długość uint32 + odstęp mogłyby
przekroczyć UINT32_MAX. Cykl liczony jest bez mnożenia całego elapsed przez
speed; pozycje nie zależą od liczby klatek. Publiczna operacja shifted draw
odrzuca offset, którego nie można bezpiecznie odjąć w signed 64-bit.
Zmiana dotyczy obu rendererów: fontu 5×8 oraz metrycznego fontu alpha 4bpp.
Nie pozostawiono aktywnego trybu, flag ani handlera powrotu w prawo.

Scenę animowaną wydłużono do 20 s, aby pokazać pełny cykl powtórzenia
przy obecnych długościach napisów. Cała diagnostyka: około 38 s i panel OFF.
Same bitmapy i wygląd fontu nie są zmieniane w tym podetapie.

## Weryfikacja

55 testów hostowych, w tym sprawdzenie monotonicznego ruchu między wrapami,
wejścia następnej kopii z prawej dla obu formatów fontów, granic uint64,
resetu, pomijania klatek i canary. Wzorce klatki dla elapsed=0 pozostają
bez zmian. ASan/UBSan testują teraz oba renderery; LeakSanitizer nadal
niedostępny pod ptrace, detect_leaks=0. Build ESP-IDF i diff check wymagane.
Brak nowych bibliotek; korzystamy z już sprawdzonych text/graphics i C11.

Odbiór nowego kierunku/cyklu na urządzeniu pozostaje HARDWARE_TEST_PENDING;
wcześniejsze potwierdzenie użytkownika dotyczyło wersji sprzed tej zmiany.
