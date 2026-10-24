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

Po sześciu obrazach dodano 8 s sceny: środkowany nagłówek, przewijane wiersze y16/24 i test LEWO/ŚRODEK/PRAWO. Łącznie około 26 s, potem panel OFF. W obecnej wersji sprawdź pauzę przy początku cyklu, wejście powtórzenia z prawej, brak zapisu poza wierszem i brak ruchu w prawo (ADR 0009). Zapisz log `marquee frames=... elapsed=... us`. To pomiar testu, nie gwarancja FPS radia.

## Duży font w animacji — 04e

Nagłówek ostatniej sceny używa StreamOO UI16 pochodzącego z Krypton. Sprawdź
„StreamOO - Zażółć gęślą jaźń” w wierszu y0..15: akcenty, ogonki, środek,
kontrast oraz brak zapisu do y16. Dolne dwie linie powinny przewijać się
z pauzami i bez wychodzenia poza swoje bounds. Sekwencja nadal trwa około
26 s i kończy się OFF. Zdjęcie/log jest potrzebne do odbioru nowego fontu;
wcześniejsze „działa” dotyczyło starszej wersji i nie zastępuje tego testu.
Wzór hostowy z implementacji: `build/krypton-layout.png` (początek animacji).

## Scroll wyłącznie w lewo — 04f

Użytkownik potwierdził, że polskie znaki i dotychczasowy scroll działają;
font na dziś akceptuje, lecz docelowy wygląd wymaga poprawy.
Nowe firmware przewija tylko od prawej do lewej i powtarza tekst po odstępie.
Przy początku cyklu zatrzymuje się na 1 s; nie wraca animacją w prawo.
Ostatnia scena trwa teraz 20 s, więc cały test około 38 s, potem panel OFF.
Sprawdź co najmniej jedno wejście kolejnej kopii tekstu z prawej i brak
wychodzenia poza wiersze. Tę nową wersję należy odebrać osobno; nie jest
objęta wcześniejszym potwierdzeniem działania.

## Odbiór scrolla 04f i widżety 04g

Użytkownik potwierdził prawidłowe działanie wersji po 04f. Nie przypisujemy
tego potwierdzenia jeszcze nieuruchomionym nowym widżetom ani pomiarom.

W nowej wersji w ostatniej 20-sekundowej scenie środek dolnego panelu
(x44/y32/132×32) zmienia się co 6 s: pasma FFT, bargraph L/R z markerami peak,
dwa wskaźniki analogowe. Oznaczenie TEST wskazuje, że dane są syntetyczne;
firmware nie odtwarza jeszcze audio i nie liczy FFT. Pełny test nadal około
38 s, potem OFF. Sprawdź wszystkie trzy tryby, czytelność obu kanałów,
brak wychodzenia poza środkowy panel i brak pozostałości przy zmianie trybu.
Scroll w lewo i fonty pozostają bez zmian. Zapisz zdjęcia/wynik jako odbiór
04g, oddzielnie od wcześniejszego potwierdzenia.

## Animowana lista TEST — 04h

Dotychczasowe działanie zostało potwierdzone przez użytkownika. Nowa lista
jest osobnym zakresem odbioru. Po 38s wcześniejszych obrazów pojawia się
8-sekundowy pokaz: zaznaczona Łódź, przejście do Gdańska i Poznania,
zatrzymanie na końcu katalogu, powrót do Gdańska i potwierdzenie, następnie
ponowne otwarcie na niezmienionym snapshotcie Łódź. Wszystkie nazwy są TEST.
Cały test trwa około 46s, potem OFF. Log: `list diagnostic requested=1003
snapshot=1002`. To rejestrator komendy testowej; nie zmienia stacji audio.

Sprawdź czytelność wybranego wiersza, płynny ruch i brak kolizji z nagłówkiem
TEST, przygaszenie sąsiadów i przejścia po zatwierdzeniu. Sterowanie jest
na razie automatyczne. Enkoder i rzeczywiste katalogi nie są tym testowane.

### Wynik użytkownika — 04h, 2026-10-24

Użytkownik potwierdził działanie listy i poprawność wyboru stacji.
Odbiór pokazu 04h na OLED zaliczony na podstawie tego zgłoszenia.
Nie jest to test fizycznego enkodera, rzeczywistego katalogu ani audio;
wersja 04h używa automatycznych zdarzeń i katalogu TEST.

## Pełny HOME — 04i

W nowym firmware 20-sekundową scenę tekstu/widżetów zastępuje kompletny
HOME. Pozostałe obrazy i 8s lista bez zmian, razem nadal około 46s i OFF.
Czasy poniżej odnoszą się do początku HOME (po sześciu obrazach statycznych):

- 0..12s: TEST: Radio Łódź, przewijany tytuł PL, MP3/192k, ikonka i -58,
  syntetyczna data/czas oraz widżety zmieniane co 4s.
- 12..15s: TEST: nowa stacja; stare tagi danych są celowo niezgodne — dwa
  wiersze metadanych mają być puste, codec/bitrate kreskami, „Brak danych”.
  Data i Wi-Fi pozostają widoczne.
- 15..18s: nowe metadane i FLAC/950k pojawiają się prawidłowo.
- 18..20s: Brak stacji, OFF, czas nieznany i brak danych audio.

TEST w dolnym prawym wierszu przypomina, że żadne z tych danych nie pochodzi
jeszcze z sieci/playera/SNTP. Sprawdź separację wierszy 16/8/8/32, czytelność
kodeka/ikonki/RSSI i zegara, brak nakładania w kolumnach 44/132/80 oraz
całkowite zniknięcie dawnych tytułów w drugiej fazie. Po HOME lista TEST działa
jak wcześniej. Zapisz wynik osobno jako odbiór 04i; nie zaliczać danych
syntetycznych jako testu połączenia Wi-Fi, zegara systemowego ani audio.

## Enkoder 05a — diagnostyka wolnych ruchów

Po wgraniu wersji stage-05a otwórz `idf.py monitor`. Nowy przełącznik
`STREAMOO_ENCODER_GESTURE_TEST` domyślnie włączony; pokaz OLED pozostaje
bez zmian i gaśnie po około 46s, diagnostyka konsoli działa nadal.

- W spoczynku sprawdź A/B/BTN i brak samoczynnych zdarzeń.
- Obracaj powoli po jednym ząbku: zapisz kierunek i liczbę ROTATE na ząbek.
  Sekwencja 11→01→00→10→11 jest dodatnia przed opcjonalnym odwróceniem.
- Krótkie naciśnięcie daje jeden SHORT przy zwolnieniu; przytrzymanie
  wyraźnie ponad 600ms daje jeden LONG przy zwolnieniu.
- Trzymaj i obracaj powoli: PRESSED_ROTATE, po zwolnieniu bez SHORT/LONG.
- Podaj wyniki, zwłaszcza dwa zdarzenia na ząbek, odwrotny kierunek lub szum.

To próbnik co tick RTOS (okres wypisany w logu), bez gwarancji szybkich ruchów.
Nie zmienia stacji, volume ani wizualizacji. NVS i audio nie są uruchamiane.
