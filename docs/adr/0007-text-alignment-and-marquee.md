# ADR 0007 — wyrównanie i przewijanie tekstu

Data: 2026-10-24. Status: część programowa 04d przyjęta,
parametry ruchu PROPONOWANE, odbiór OLED HARDWARE_TEST_PENDING.

Tekst mieszczący się w bounds jest wyrównywany left/center/right. Nominalna
szerokość obrazu wynosi 6×liczba skalarów minus końcowy odstęp 1px. Zachowano
streamoo_text_width jako pomiar advance, aby nie zmieniać dotychczasowego API.
Długi tekst przewija się ping-pong: pauza na początku, ruch, pauza na końcu,
powrót. Funkcja offsetu jest czysta i bazuje na upływie czasu, nie liczbie
klatek. Po zmianie tekstu/stacji właściciel UI podaje elapsed=0. Nie dodano
jeszcze player/UI state ani połączenia z metadanymi sieciowymi.

Proponowane do testu: 30 px/s, pauza 1000 ms, rendering co 40 ms. speed=0
wyłącza ruch. Parametry są argumentami API, bez zaszywania ich w silniku.
Obliczenia cyklu są 64-bit, offset 32-bit; pozycja przed clippingiem 64-bit,
aby długi tekst nie zawijał współrzędnych int16. Backend nadal nie alokuje,
nie zależy od sterownika/RTOS i nie kopiuje ani nie zmienia źródła UTF-8.
Canvas potomny izoluje wiersz od innych widgetów.

Po sześciu dotychczasowych obrazach diagnostyka dodaje 8 s sceny z dwiema
przewijanymi liniami i trzema wyrównaniami. Całość około 26 s, potem OFF.
Zadanie loguje faktyczną liczbę klatek i czas animacji. To test tekstu, nie
radio. Wskaźnika ~25 FPS nie uznaje się za zmierzony przed testem na PCB.

Sześć nowych testów obejmuje fazy ruchu, granice uint, fitting/speed=0,
wyrównanie, pionowy clipping, offset >32767, pomijane klatki, reset oraz
stałość nagłówka przy przewijanych metadanych. Nowe API korzysta z już
sprawdzonego text/graphics i standardowej biblioteki C ESP-IDF 6.1. Brak
nowych zależności zewnętrznych. Font Krypton będzie osobnym podetapem.

Na polecenie użytkownika wszystkie nowe commity mają trailery AI-Model i
AI-Tokens. Nieudostępnione metryki pozostają unavailable, a nie szacunkiem;
procedura jest opisana w `docs/COMMIT_METRICS.md` i AGENTS.md.
