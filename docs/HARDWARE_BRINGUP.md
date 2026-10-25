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
   5/6/7 (sprzęt użytkownika; schemat referencyjny różni się); I²S 48/47/21. Potwierdź podciąganie A/B/przycisku na PCB.
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

## Enkoder 05b — rozstrzygnięcie braku kanału B

Wersja 05b zastępuje powyższy polling przechwytywaniem przerwań. Po wgraniu
monitor powinien wypisać `TEST IRQ capture: A=GPIO5 B=GPIO16 BTN=GPIO7; half=1`.
Log co sekundę ma teraz nazwę `IRQ edges`, zamiast `sampled edges`.

Obracaj kilka ząbków w jedną stronę, potem w drugą. Liczniki IRQ A i B
powinny rosnąć, a TEST ROTATE delta powinno mieć przeciwne znaki dla obu
kierunków. W spoczynku wartości A/B mogą być 0 lub 1; istotne są zbocza.
Jeżeli pojawia się ostrzeżenie `A changes, B has no IRQ`, prześlij log;
należy zweryfikować tor B do GPIO16. Nie zmieniono pinów ani pull-up.
Półkrok pozostaje włączony zgodnie z potwierdzeniem użytkownika.

Liczba IRQ nie jest liczbą ząbków (obejmuje odbicia). `IRQ queue overflow`
unieważnia próbę: dekoder resetuje się bez wymyślania brakujących kroków.
Sprawdź też SHORT/LONG oraz PRESSED_ROTATE bez kliku po puszczeniu.

## Korekta połączenia enkodera — 5/6/7

Użytkownik ustalił B na GPIO6 i po zmianie przekazał log z IRQ obu kanałów,
ROTATE delta=-1/+1, SHORT, LONG i PRESSED_ROTATE. Poprzednia instrukcja
05b z B=GPIO16 dotyczy błędnego dla tego sprzętu przypisania referencyjnego.
Aktualne domyślne A/B/BTN: 5/6/7, HALF_QUADRATURE=y. Istniejący sdkconfig
może zachować starą wartość: sprawdź Encoder B GPIO=6 w menuconfig.
Nie ustalono z logu liczby zdarzeń na ząbek ani kierunku względem obrotu
zegarowego. Sekwencja PRESSED_ROTATE, następnie ROTATE nie mówi, czy
przycisk pozostawał fizycznie wciśnięty — sprawdź to przy odbiorze gestów.

## Enkoder 05c — sterowanie ekranem

Domyślnie `STREAMOO_ENCODER_UI_TEST=y`: HOME pojawia się od razu,
bez 46s automatycznej sekwencji, i pozostaje do resetu. Konsola wypisuje
`Interactive encoder TEST ready`. Wyłączenie tej opcji przywraca dawny pokaz.
GPIO A/B/BTN=5/6/7, półkrok; tryb A domyślny.

1. Obracaj na HOME: TEST VOL startuje od 50 i zatrzymuje się na 0/100.
2. Trzymaj przycisk i obracaj: zmienia się widżet oraz W1/W2/W3,
   volume pozostaje takie samo. Zwolnienie nie powinno otwierać listy.
3. Krótki klik otwiera listę; obrót przemieszcza środkowy wybór z animacją.
   Lista nie zawija się. Samo przeglądanie nie zmienia testowej stacji.
4. Długi klik anuluje i wraca do poprzedniej stacji. Ponownie otwórz listę,
   zmień wybór i krótko kliknij: HOME pokazuje wybraną testową stację.
5. Długi klik na HOME przełącza TEST PLAY/STOP; w STOP poziomy syntetyczne
   spadają do zera. Nie ma jeszcze prawdziwego odtwarzania.
6. Opcjonalnie tryb B w menuconfig: krótki na HOME przełącza PLAY/STOP,
   długi otwiera listę. Gesty na liście pozostają jak w punktach 3/4.

Przekaż liczbę przesunięć na jeden ząbek i kierunek względem obrotu zegarowego.
Przy `IRQ queue overflow` lub `Gesture queue full` próba transportu nie jest
zaliczona; podaj log. NVS nie zapisuje stanu — reset przywraca dane TEST.

## Preferencje 05d — zapis i restart

1. Po wgraniu zmień volume na rozpoznawalną wartość, np. 37, a widżet na W2.
2. Od ostatniej zmiany odczekaj co najmniej 5s. W monitorze oczekuj
   `preferences: Saved volume=37 widget=2` (drobne opóźnienie harmonogramu możliwe).
3. Zresetuj płytkę bez erase-flash. HOME powinien odtworzyć VOL 37 i W2,
   a log startowy preferences podać te wartości. Stacja/PLAY pozostają TEST.
4. Zmieniaj volume co 1–2s: zapis powinien nastąpić dopiero po 5s spokoju.
5. Nie dotykaj enkodera przez kolejne 10s: bez ponownych komunikatów Saved.
6. Zmień i wróć do poprzedniej zapisanej wartości przed upływem 5s:
   nie powinno być zbędnego zapisu, o ile nic innego się nie zmieniło.

Reset wcześniej niż Saved może przywrócić poprzednie wartości. Wersja nie
ma jeszcze kontrolowanego shutdown/flush. Jeśli pojawi się Preferences
unavailable lub Save failed, prześlij log; firmware nie kasuje NVS automatycznie.
Nie testowano jeszcze zaniku w trakcie samego zapisu ani zużycia flash.

## Audio 06a — lewy/prawy PCM5102A

Nowy STREAMOO_I2S_SMOKE_TEST domyślnie y uruchamia jednorazowy ton po boot.
Nie kasuj NVS; UI nadal ma zapisane preferencje. Sygnał ma stały niski poziom
około -30dBFS peak niezależny od TEST VOL i TEST PLAY/STOP. Do odsłuchu
użyj wejścia liniowego wzmacniacza/aktywnych głośników z niską głośnością.

Sekwencja względem startu I2S (początkowe DMA dodaje około20ms ciszy):

| Czas | Wyjście |
|---|---|
| 0–1s | cisza |
| 1–3s | tylko lewy, 1kHz |
| 3–4s | cisza |
| 4–6s | tylko prawy, 2kHz |
| 6–7s | cisza |
| 7–9s | oba kanały, 1kHz |
| 9–10s | cisza, następnie wyłączenie I2S |

Sprawdź kolejność stron, oba kanały, ciche przejścia i brak trwającego tonu
po końcu. Podaj log `audio_probe: TEST ended: ... buffers=... max_fill=... invalid=...`.
Oczekiwane ESP_OK, invalid=0, max_fill wyraźnie poniżej5000us. To pomiar
renderowania w callbacku, nie dowód braku jitter/dropout. Zmiany enkodera,
UI i zapis NVS można sprawdzić równolegle, ale testu sygnału nie zatrzymuje
przycisk PLAY/STOP. Wyłącz opcję I2S smoke, aby boot nie emitował tonów.

Przy ciszy zweryfikuj faktyczne DOUT48→DIN, BCLK47→BCK, WS21→LRCK,
wspólną masę i zasilanie zgodne z dokumentacją swojego modułu. Wymagania
profilu PCM5102A: FMT=I2S, XSMT odmutowany, SCK na GND dla pracy bez MCLK.
Źródło: [TI SLAS859C](https://www.ti.com/lit/ds/symlink/pcm5102a.pdf).
Nie przypisano dodatkowych GPIO do pinów sterujących DAC. Schemat ma
również złącze alternatywnego modułu WeAct PCM5100A; nie zakładamy użycia
tamtego modułu. Jeśli sygnału brak, potrzebny model/zdjęcie zworek modułu.

Dla analizatora: WS nominalnie48kHz, BCLK3.072MHz, Philips bit-shift1,
stereo32. Rzeczywisty rate/jitter i amplituda analogowa wymagają pomiaru.

## Audio06b — rzeczywiste VOL i PLAY/STOP

Domyślnie STREAMOO_AUDIO_CONTROL_TEST=y ma pierwszeństwo nad jednorazowym
testem06a. Po boot HOME ma pokazać STOP i volume zapisane w NVS. Brak tonu
na starcie jest oczekiwany. Tryb A: długi klik przełącza PLAY/STOP.

1. Ustaw np. VOL80–100 i włącz PLAY długim kliknięciem. Oczekiwany ciągły
   ton1kHz na obu kanałach. Jego maksimum przy100 jest tak niskie jak w06a;
   przy50 dochodzi około30dB tłumienia, więc może być słabo słyszalny.
2. Obrót w prawo/lewo ma rzeczywiście zmieniać poziom; VOL0 ma być ciszą.
3. Kilka szybkich zmian volume oraz PLAY/STOP: słuchaj, czy nie ma trzasków.
   Profil rampy to50ms plus opóźnienie DMA/sterowania, jeszcze niezmierzone.
4. Trzymaj i obracaj: widżet się zmienia, poziom tonu pozostaje taki sam.
   Przeglądaj/anuluj listę: ton trwa. Wybór testowej stacji zmienia jej nazwę,
   nie częstotliwość ani prawdziwy strumień. Widżety są syntetyczne.
5. Po zmianie volume/widżetu odczekaj Saved, zresetuj. Volume/widżet wracają,
   ale audio startuje w STOP. Nie kasuj NVS.

Podeślij wynik odsłuchu i log `audio_control: buffers=... max_fill=... invalid=...`.
Oczekiwany postęp buforów, invalid0 i max_fill wyraźnie poniżej5000us.
To nie jest pomiar całego ISR, jitter ani dowód braku dropout.
TrybB: krótki klik PLAY/STOP, długi lista (gesty na liście bez zmian).
Wyłącz AUDIO_CONTROL_TEST, aby wrócić do wcześniejszej sekwencji06a
(przy nadal włączonym I2S_SMOKE_TEST).
