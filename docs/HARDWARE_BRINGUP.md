# Protokół bramy sprzętowej — HARDWARE_TEST_PENDING

Nie wykonano flashowania, kasowania flash ani operacji eFuse.
Potwierdzenie bazowego uruchomienia N16R8 przekazane przez użytkownika
pozostaje ważne; nie trzeba go powtarzać jako odrębnego etapu.
Poniżej sprawdzamy nowe kontrole startowe oraz niepotwierdzone połączenia.

## Przygotowanie

1. Moduł został zidentyfikowany: msalamon OLED_312_YELLOW, 3,12 cala,
   SSD1322 256×64. Użytkownik potwierdził R5/R8 (BS1=0, BS0=0), czyli 4SPI.
   Nie przelutowywać na R6/R8 według błędnie podpisanego zdjęcia poradnika.
   Źródła i profil eksperymentalny: `adr/0003-ssd1322-msalamon-spi-bringup.md`.
   Odbiór resetu EN, parametrów panelu i obrazu nadal wymaga płytki.
2. Przy wyłączonym zasilaniu sprawdź ciągłość połączeń z tabelą
   `02_HARDWARE.md`: OLED 11/12/9/10, RESET bez osobnego GPIO; encoder
   5/16/7; I²S 48/47/21. Potwierdź podciąganie A/B/przycisku na PCB.
3. W terminalu ESP-IDF 6.1 uruchom:

   ```sh
   idf.py menuconfig
   python3 tools/check_config.py sdkconfig --platform
   idf.py reconfigure
   idf.py build
   ```

   W `StreamOO Configuration` zachowaj profil PCB. Opcjonalnie włącz
   `Diagnostic raw encoder GPIO probe`. Podciąganie wewnętrzne włączaj
   zgodnie z potwierdzonym obwodem, bez zgadywania.

## Wgranie i obserwacja — czynność użytkownika

Po sprawdzeniu połączeń użytkownik może uruchomić:

```sh
idf.py -p PORT flash monitor
```

Firmware inicjalizuje OLED tylko z włączonym `STREAMOO_LCD_SMOKE_TEST`
(domyślnie włączony dla etapu 04a). Nie inicjalizuje I²S i nie jest radiem.
Oczekiwane nowe komunikaty:

- `board: N16R8 GPIO map validated`;
- `platform`: IDF 6.1, dwa rdzenie, flash 16777216, PSRAM 8388608;
- metryki pamięci co 10 sekund, bez restartów i błędów startu;
- jeśli włączono probe: `input_probe` z A/B/BTN i liczbą zmian co sekundę.

Obróć enkoder powoli w obie strony, następnie naciśnij i zwolnij przycisk.
Zapisz stany spoczynkowe i wciśniętego przycisku; A i B powinny zmieniać
poziomy. Dryf stanów bez dotykania oznacza potrzebę kontroli podciągania.
Nie porównuj liczby zmian z liczbą detentów — polling może gubić impulsy.
Kierunek, half/full quadrature i debounce nie są tym eksperymentem zatwierdzane.
Wyłącz probe po pomiarach; nie jest docelowym sterownikiem wejścia.

## Wynik do zapisania

- commit firmware, wersja modułu i PCB, opcje menuconfig;
- log rozruchu oraz minimum 30 s metryk pamięci;
- mapa sprawdzonych połączeń, napięcia i dokumentacja resetu OLED;
- poziomy A/B/BTN, obecność podciągania, ewentualne niestabilne sygnały;
- PASS/FAIL/OPEN osobno dla każdej pozycji.

SSD1322: sterownik i wzór są gotowe do testu, odbiór obrazu/skali 0..15
i pomiar czasu SPI nadal HARDWARE_TEST_PENDING. Nie zaliczać po samym buildzie.
Audio i testy funkcjonalne gestów pozostają w kolejnych etapach.

## Odbiór SSD1322 — etap 04a

GPIO pozostają zgodne z PCB, bez dodatkowego pinu resetu:

| Sygnał modułu (pin listwy J1 w schemacie sprzedawcy) | StreamOO |
|---|---|
| D0/CLK (4) | SCLK GPIO12 |
| D1/DIN (5) | MOSI GPIO11 |
| D/C# (14) | GPIO10 |
| CS# (16) | GPIO9 |
| RES# (15) | CHIP_PU/EN, brak GPIO; sprawdzić na płytce |
| GND (1), VCC_IN (2) | istniejące zasilanie PCB; logika wyłącznie 3,3 V |

Schemat modułu nie jest schematem połączeń całej PCB. Nie zmieniaj okablowania
bez porównania z `references/YR.kicad_sch`; również pozostałe piny interfejsu
muszą mieć stany zgodne z tabelą 8-1 kontrolera (nie zostawiać wymaganych
wejść LOW w stanie pływającym).

W menuconfig zachowaj offset=28 i remap=20 jako punkt startowy eksperymentu.
Po buildzie i ręcznym wgraniu przez użytkownika wykonaj pełne wyłączenie
oraz włączenie zasilania, aby objąć testem reset modułu; samo monitor/reset
procesora nie dowodzi poprawnego resetu OLED.

Oczekiwany przebieg (każdy obraz około 3 s):

1. Czerń (`step=0`).
2. Biel przy ograniczonym prądzie testowym (`step=1`).
3. 16 pionowych pasów od ciemnych po lewej do jasnych po prawej, ramka,
   jasny mały znacznik lewy górny i ciemniejszy prawy dolny (`step=2`).
   Pierwszy wiersz powtarza krótką rampę co 16 pikseli, wykrywając zamianę nibble.
4. Negatyw wzoru (`step=3`) — po nim w wersji 04b dochodzi scena canvas.

Podgląd samego wzoru (liniowe mapowanie kodów, bez symulowania gamma OLED):

```sh
python3 tools/preview_ssd1322.py build/ssd1322-smoke.png
```

Zapisz zdjęcie wzoru, pełne logi `display_smoke` z `frame transfer=... us`,
poprawność obu krawędzi, orientacji, brak pasów/przesunięć oraz zachowanie
po zimnym starcie i po resecie procesora. Typowo sam payload 8192 B przy
10 MHz wymaga minimum 6,554 ms; rzeczywisty czas zawiera dodatkową pracę.
Nie jest to wynik pomiaru. Domyślna tabela B9 nie gwarantuje jednakowej
odległości jasności między kodami ani wyraźnego rozróżnienia GS0/GS1.

Przy braku obrazu zapisz log, sprawdź zasilanie, reset oraz analizatorem
SCLK/MOSI/CS/DC. D/C musi być LOW dla kodu komendy i HIGH dla parametrów
oraz danych. Nie zmieniaj losowo offsetu, napięć ani parametrów analogowych.
Przy błędnej orientacji raportuj położenie obu znaczników przed zmianą remap.

Przed dalszym odbiorem renderera należy potwierdzić ten test na rzeczywistym
panelu. Fonty i UI nie są testowane wzorem diagnostycznym.

## Potwierdzenie i dodatkowa scena 04b

2026-10-24 użytkownik potwierdził, że po wgraniu firmware wyświetlacz działa.
Zaliczamy podstawowe uruchomienie; bez logów i szczegółowego opisu nie
zaliczamy osobno pomiarów, orientacji, gamma ani sekwencji zimnego startu.

Nowy firmware po negatywie wyświetla jeszcze `step=4` przez około 3 s:
ramki w układzie 16/8/8/32, trzy dolne kolumny, poziome linie przycięte do
bounds, przekątne wyłącznie środkowego dolnego pola oraz strzałkę u góry.
To test prymitywów i izolacji pól, nie ekran radia. Całość trwa około 15 s,
potem panel gaśnie. Log `canvas scene raster=... us` podaje czas samej
rasteryzacji; `frame transfer=... us` pozostaje pomiarem wysyłki.
Tego nowego obrazu nie obejmuje wcześniejsze potwierdzenie użytkownika.

## Szósty obraz — font PL (04c-2)

Po scenie canvas nowy firmware wyświetla przez około 3 s `step=5`:
małe i wielkie polskie litery, „Zażółć gęślą jaźń” oraz pary ą/Ą/ę/Ę
na ostatnim wierszu. Diagnostyka trwa teraz około 18 s i kończy się OFF.
Sprawdź wszystkie 18 liter, rozróżnienie ś/ź/ż oraz pełne ogonki bez
nakładania na kolejny wiersz. Zapisz zdjęcie/log i wynik osobno od
wcześniejszego potwierdzenia podstawowego działania OLED.
Podgląd oczekiwany: `build/polish-text-smoke.png` z ostatniej implementacji.
To test bitmapowego fontu, nie fontu Krypton ani gotowego UI.

### Wynik zgłoszony przez użytkownika — 2026-10-24

Po wersji 04c-2 (`867714d`) użytkownik potwierdził własną kompilację,
wgranie i działanie firmware. Zapisano podstawowy test funkcjonalny jako
potwierdzony przez użytkownika. Szczegółów oceny glifów i pomiarów nie
przekazano; nie uzupełniono ich domysłami. Dalsza implementacja tekstu
może być kontynuowana.

## Animacja tekstu — 04d

Po sześciu obrazach dodano 8 s sceny: środkowany nagłówek, przewijane wiersze y16/24 i test LEWO/ŚRODEK/PRAWO. Łącznie około 26 s, potem panel OFF. Sprawdź pauzę przy początku i końcu, brak zapisu poza wierszem oraz brak gwałtownych skoków. Zapisz log `marquee frames=... elapsed=... us`. To pomiar testu, nie gwarancja FPS radia.

## Duży font w animacji — 04e

Nagłówek ostatniej sceny używa StreamOO UI16 pochodzącego z Krypton. Sprawdź
„StreamOO - Zażółć gęślą jaźń” w wierszu y0..15: akcenty, ogonki, środek,
kontrast oraz brak zapisu do y16. Dolne dwie linie powinny przewijać się
z pauzami i bez wychodzenia poza swoje bounds. Sekwencja nadal trwa około
26 s i kończy się OFF. Zdjęcie/log jest potrzebne do odbioru nowego fontu;
wcześniejsze „działa” dotyczyło starszej wersji i nie zastępuje tego testu.
Wzór hostowy z implementacji: `build/krypton-layout.png` (początek animacji).
