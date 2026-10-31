# 04 — SSD1322, UI, wymienne widżety

**USTALONE:** SSD1322 256×64, grayscale 4bpp (16 jasności), framebuffer 8192 B, własny renderer bez LVGL. Wszystkie widgety dostają prostokąt `bounds`, źródło danych/model i obiekt canvas; nie zapisują się do sterownika bezpośrednio. Clipping i izolacja aktualizacji to obowiązek silnika. Docelowo testowalny renderer hostowy.

## Geometria głównego ekranu
| X | Y | Rozmiar | Obiekt |
|---|---|---|---|
| 0 | 0 | 256×16 | nazwa stacji wyśrodkowana, Krypton |
| 0 | 16 | 256×8 | tytuł utworu, bitmapa 5×7 UTF-8 PL |
| 0 | 24 | 256×8 | dodatkowy tekst, bitmapa 5×7 UTF-8 PL |
| 0 | 32 | ~44×32 | kodek, bitrate, ikona Wi-Fi w dolnym lewym rogu |
| ~44 | 32 | ~132×32 | wymienna wizualizacja audio |
| ~176 | 32 | ~80×32 | data i zegar |

Szerokości kolumn dolnego panelu 44/132/80 są **PROPONOWANE** — dopasuj po testach czytelności, zachowując łączną szerokość 256 i minimum na dwa analogowe mierniki.

## Zachowanie
- Pierwszy wiersz nazwa stacji centrowana, 16 px; pozostałe linie scroll poziomy gdy tekst się nie mieści (marquee z czasem spoczynku, bez gwałtownych skoków).
- Niższy panel: RSSI Wi-Fi jako mała ikona, nad nią kodek i bitrate; po prawej data i zegar; środek to FFT/2×L/R bargraph/2×analog.
- Wykryj utratę metadanych i resetuj tekst poprzedniej stacji po przełączeniu.
- Wybór stacji: pionowa animowana lista z wyróżnioną **środkową pozycją** (najwyższa jasność), wybór odbywa się dopiero po zatwierdzeniu, aktualna stacja nadal gra.
- Parametry docelowe: ~25 FPS efektów, auto redukcja przy obciążeniu, ograniczenie wysyłania bufora przez SPI przy możliwości.

## Fonty
- Monaspace Krypton (OFL 1.1) do większych tekstów; zasoby z TTF generowane przez skrypt Python do tablic C i podglądu PNG. Zachowaj OFL/noty prawne. Nie dołączaj cudzych fontów bez weryfikacji licencji.
- Drobny tekst: źródłowa bitmapowa 5×7 ze wsparciem polskich znaków (wiersz 8px). Dokładny plik/odmiana do potwierdzenia z lokalnego zasobu użytkownika; Unicode→glyph mapping jawne, nie podmieniaj fondu w bibliotece Arduino.

## Testy
Golden-image framebuffer, napisy PL `ąćęłńóśźż`, clipping, wyrównanie 4bpp, scroll, 3 wizualizacje, gest przełączania, pomiar czasu pełnego transferu SSD1322.

## Część 04a — sterownik i eksperyment sprzętowy

Zaimplementowano `ssd1322`: protokół host-testable z callbackiem transportu,
adapter natywnego SPI ESP-IDF i API pełnej klatki 4bpp. Zadanie aplikacji
`display_smoke` pokazuje skończoną sekwencję testową i raportuje czas transferu.
Moduł użytkownika i profil do akceptacji: [ADR 0003](adr/0003-ssd1322-msalamon-spi-bringup.md).
Domyślnie test startuje po walidacji platformy, następnie wyłącza panel.
Można go wyłączyć przez `STREAMOO_LCD_SMOKE_TEST` w menuconfig.

Renderer, fonty i docelowe widgety nie zostały tym etapem zaimplementowane.
Odbiór SPI/obrazu: [HARDWARE_TEST_PENDING](HARDWARE_BRINGUP.md).

## Część 04b — canvas

Użytkownik potwierdził podstawowe działanie OLED po wgraniu firmware 04a.
Zaimplementowano niezależny `graphics`: 4bpp, stride/padding, prymitywy,
maski 1bpp, clipping i canvas potomny dla widżetów. Bez zależności od sterownika,
RTOS i DSP. [ADR 0004](adr/0004-independent-4bpp-canvas.md) opisuje kontrakt,
testy, limity i wynik sanitizera.

Scena geometryczna jest piątym obrazem testu OLED (całość około 15 s).
Jej rasteryzacja i transfer mają osobne metryki. Nowy obraz na urządzeniu
pozostaje HARDWARE_TEST_PENDING. Nie jest to ekran radia ani test fontów.

## Część 04c-1 — zidentyfikowane polskie glify i backend tekstu

Użytkownik wskazał `glcdfont.c`; potwierdzono komplet 18 polskich liter
oraz konieczność zachowania ósmego wiersza. [ADR 0005](adr/0005-font-reference-and-utf8.md)
zawiera mapowanie i wynik sprawdzenia licencji. Bitmap nie importowano:
warunki polskich zmian nie są opisane w ZIP i wymagają wyjaśnienia.

Niezależny `text` obsługuje ścisłe UTF-8, pomiar szerokości i provider 5×8,
bez kopiowania konwertera yoRadio. Testy syntetyczne nie oznaczają wdrożenia
polskiego fontu, fontu Krypton ani tekstów na urządzeniu.

## Część 04c-2 — font PL dołączony

Na wyraźne polecenie użytkownika zaimportowano ASCII i 18 polskich glifów
z ustalonego źródła. [ADR 0006](adr/0006-provisional-polish-font-import.md)
oraz `THIRD_PARTY_NOTICES.md` odnotowują licencję polskich zmian jako
DO SPRAWDZENIA. Nie jest to import kodu Arduino ani konwertera yoRadio.
Szósty obraz diagnostyczny zawiera polskie napisy; cały test trwa około 18 s.
Odbiór na OLED nadal HARDWARE_TEST_PENDING, font Krypton i marquee są kolejne.

## Część 04d — wyrównanie i marquee

Zaimplementowano wyrównanie i przewijanie opisane w [ADR 0007](adr/0007-text-alignment-and-marquee.md). Ruch zależy od czasu i nie modyfikuje UTF-8. Pierwotny ping-pong został zastąpiony wymaganiem ruchu tylko w lewo w ADR 0009. Na końcu testu OLED dodano 8 s animacji; pełna sekwencja trwa około 26 s. Parametry i płynność wymagają testu PCB.

## Część 04e — większy font i alpha

W nagłówku testu używany jest StreamOO UI16, podzbiór OFL z Monaspace Krypton
v1.400, 113 glifów, linia 16px. [ADR 0008](adr/0008-krypton-and-alpha-font-rendering.md)
opisuje format, licencję, baseline i kompozycję 4bpp. Wiersze y16/y24 nadal
mają mały font PL. Wzorzec hostowy i build przechodzą, odbiór na OLED pending.

## Część 04f — tylko od prawej do lewej

Użytkownik potwierdził poprawne polskie znaki i działanie scrolla; obecny
font tymczasowo akceptuje, ale jego wygląd pozostaje do dopracowania.
[ADR 0009](adr/0009-left-only-scroll.md) ustala scroll wyłącznie w lewo.
Tekst powtarza się z prawej po proponowanym odstępie 24px, bez cofania.
Animacja testu trwa teraz 20 s (całość około 38 s). Nowy cykl wymaga testu OLED.

## Część 04g — wymienne widżety

Nowa warstwa `ui_widgets` renderuje spectrum, bargraph L/R z peak oraz dwa
mierniki analogowe przez wspólny canvas/bounds/model. [ADR 0010](adr/0010-visualization-widgets.md)
określa API i granice odpowiedzialności. Rysowanie nie wykonuje analizy DSP.
Aplikacyjna diagnostyka automatycznie zmienia widżet co 6 s, z jawnymi danymi
TEST; to nie jest jeszcze wybór gestem ani działająca wizualizacja audio.
Użytkownik potwierdził starszy scroll 04f; nowe widżety oczekują odbioru OLED.

## Część 04h — lista i właściciel stanu

`ui` zarządza przeglądaniem listy, logicznym wyborem i pozycją animacji.
Renderer pokazuje wyróżniony cel i przygaszonych sąsiadów; nie wywołuje
sterownika ani playera. Confirm przekazuje stable_id+generation do adaptera
kolejki, cancel/browse nie wysyłają komendy. [ADR 0011](adr/0011-ui-owner-and-station-list.md)
opisuje własność snapshotów, zachowanie przy odmowie kolejki i animację.

Testowa lista nie zawija się, D19 pozostaje OPEN. Automat diagnostyczny
pokazuje ją przez 8s; pełny test około 46s. Odbiór listy OLED pending,
sterowanie enkoderem i pełny ekran HOME nie są jeszcze zaimplementowane.

## Część 04i — pełny HOME

`streamoo_ui_update_home` przyjmuje pełny spójny snapshot, a draw_home składa
widok bez dostępu do playera/sieci/RTC. [ADR 0012](adr/0012-home-snapshot-renderer.md)
opisuje format, własność, generacje i reset tekstów. Tytuł, codec i pomiary
nie mogą pochodzić ze starszej stacji/strumienia. Nieznane dane mają jawne
placeholdery. UI wybiera rodzaj widżetu, a producent dostarcza poziomy.

Nowy 20s pokaz HOME zastępuje poprzednią scenę i zachowuje czas całej
sekwencji około 46s. Użytkownik potwierdził poprawne działanie pokazu HOME
wersji `8f895bd` na OLED (2026-10-24). Renderery i lista są gotowe
programowo; pełne fizyczne sterowanie wymaga etapu 05 i źródła danych dalszych etapów.

09b: ekran AP z rzeczywistym SSID/hasłem i URL ma pierwszeństwo nad HOME.
Pełny klucz do63 znaków w dwóch wierszach42+21, bez ucinania. VOL/PLAY/STOP
z enkodera nadal działają, lista/widżet nie otwierają się w tle. Po AP off
powrót do HOME. Hasło nie jest częścią snapshotów HTTP. ADR0027.

Nowsza polityka ADR0028: AP jest wyłącznie konfiguracją, bez odtwarzania.
Enkoder na ekranie AP ignorowany także dla VOL/PLAY/STOP; dane AP pozostają
widoczne. Poza AP zachowanie HOME/enkodera bez zmian. Zastępuje poprzednią
obsługę audio na ekranie AP z eksperymentu09b.
