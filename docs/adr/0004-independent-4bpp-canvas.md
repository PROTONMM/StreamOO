# ADR 0004 — niezależny canvas 4bpp

Data: 2026-10-24. Status: część programowa 04b przyjęta; nowy obraz na OLED
HARDWARE_TEST_PENDING, pełne UI pozostaje niezrealizowane.

## Kontekst i decyzja

Użytkownik poinformował po wgraniu firmware, że wyświetlacz działa. Jest to
potwierdzenie podstawowego działania, nie kompletny protokół pomiarów SPI,
orientacji, gamma ani resetu. Pozwala kontynuować implementację renderera.

`graphics` zawiera oryginalny kod C11, bez zewnętrznych bibliotek i nowych
zależności licencyjnych. Korzysta wyłącznie ze standardowych nagłówków C
(stdint/stddef/stdbool/stdlib/string) dostępnych w toolchain ESP-IDF 6.1;
build docelowy oraz kompilacja hostowa weryfikują dostępność. Nie zależy od
SSD1322, SPI, RTOS, DSP, LVGL ani Arduino. Właściciel dostarcza pamięć,
format: row-major, 4bpp, starszy nibble pierwszy. Dopuszczone są nieparzyste
szerokości i stride większy od aktywnej szerokości. Padding nie jest zmieniany.

API: piksel, wypełnienie/obrys prostokąta, linia Bresenhama, transparentna
maska 1bpp dla przyszłych glifów. Wszystkie operacje respektują clip.
Canvas potomny dziedziczy przecięcie clip rodzica i bounds widżetu; współrzędne
pozostają w układzie całej powierzchni, rodzic się nie zmienia. Widżet dostaje
canvas potomny; aplikacja przekazuje dopiero gotowe bytes do sterownika.
Wywołanie canvas_clip zastępuje clip w obrębie stałych bounds danego canvas;
widżet nie może zresetowaniem clip wyjść poza odziedziczone bounds. Do
zagnieżdżania należy tworzyć canvas potomny.

Wymiary i współrzędne: signed 16-bit, obliczenia pośrednie signed 32-bit,
indeksy pamięci size_t. Niepoprawny init zeruje uchwyt; niepoprawne kolory,
puste prostokąty i obszary poza clip nie piszą do pamięci. API nie chroni przed
ręcznym uszkodzeniem publicznej struktury ani równoległym zapisem; jeden
właściciel powierzchni, podobnie jak dla sterownika. Maska źródłowa nie może
nakładać się na framebuffer. Brak malloc, float i ukrytego stanu globalnego.
Fill używa memset dla całych bajtów i chroni sąsiednie nibbles na krawędziach.
Linia ma górną granicę 65536 kroków dla skrajnych endpointów; nie deklarujemy
wydajności przed pomiarem na ESP32-S3.

## Integracja i dowody

Aplikacyjna scena `canvas_smoke` prezentuje wyłącznie geometrię 16/8/8/32
z proponowanymi szerokościami 44/132/80, clipping, maskę i linie. Nie symuluje
stacji, metadanych, zegara ani audio. Wzór jest piątym obrazem diagnostycznym,
całość trwa teraz około 15 s. Loguje czas rasteryzacji i transferu oddzielnie.
Renderer nie zna wymiarów SSD1322; scena aplikacji wybiera 256×64.

9 nowych testów hostowych obejmuje niezależną wyrocznię pikselową dla 600
prostokątów, nibbles/padding, zagnieżdżone clip, obrys częściowo poza ekranem,
linie wszystkich oktantów i endpointy INT16_MIN/MAX, maski i walidację.
Zamrożony obraz 8192 B `tests/fixtures/canvas_smoke.4bpp` porównuje całą
klatkę sceny. Wzorzec wygenerowano raz z kodu C i obejrzano jako PNG przed
zapisem; test nigdy nie regeneruje wzorca automatycznie. Canary chronią boki
bufora. Dodatkowy program `tests/canvas_sanitizer.c` wykonuje 10000 sekwencji
z ASan/UBSan. LeakSanitizer w tym środowisku nie działa pod ptrace; test
uruchomiono z detect_leaks=0. Nie jest to test wycieków; renderer nie alokuje.

## Otwarte

D14: źródło konkretnej czcionki 5×7 PL nadal wymaga informacji użytkownika.
Maska 1bpp nie jest implementacją czcionki ani obsługi Unicode. Krypton,
metadane, marquee, animacja listy i widżety DSP należą do dalszych części 04.
Nie uznajemy całego etapu za zakończony po samych prymitywach rysowania.
