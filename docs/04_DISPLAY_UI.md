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
