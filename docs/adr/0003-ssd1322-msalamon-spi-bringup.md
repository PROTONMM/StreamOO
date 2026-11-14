# ADR 0003 — moduł OLED msalamon i test SSD1322 przez SPI

Data: 2026-10-24. Status: przyjęte dla części programowej etapu 04a;
profil obrazu PROPONOWANY, odbiór HARDWARE_TEST_PENDING.

## Dane wejściowe i rozstrzygnięcie interfejsu

Użytkownik wskazał moduł msalamon OLED 3,12 cala, żółty, SKU
`OLED_312_YELLOW`, 256×64 SSD1322, i dwukrotnie potwierdził obsadzenie R5/R8.
Według oznaczeń na zdjęciu PCB R5 wybiera BS1=0, R8 wybiera BS0=0.
Tabela 7-2 dokumentacji SSD1322 rev.1.2 przypisuje 00 do 4-wire SPI.
Nie ma podstaw do zalecania zmiany na R6/R8: oznaczenia zdjęć poradnika
sprzedawcy są niespójne z trybami kontrolera. Numeracja rezystorów schematu
modułu i zdjęć może się różnić; rozstrzygające są BS1/BS0.

Strona produktu określa zasilanie modułu 3,3–5 V i logikę 3,3 V.
Nie zmienia to napięć i połączeń PCB StreamOO. RESET=-1 nadal oznacza
reset dostarczony przez płytkę (CHIP_PU/EN), bez sterowania GPIO z firmware.

## Implementacja

`ssd1322_protocol` to niezależny kod C z interfejsem write(context, data,
bytes, length). Parametry komend mają D/C=1. Init pozostawia panel OFF;
caller najpierw zeruje RAM, potem włącza obraz. Pełna klatka ma dokładnie
8192 bajty, row-major, starszy nibble reprezentuje pierwszy piksel.
Adres kolumn jest w jednostkach czterech pikseli; zakres to offset..offset+63,
a wierszy 0..63. Błędy transmisji natychmiast wracają do caller.

Adapter `streamoo_ssd1322` używa natywnego `esp_driver_spi` i GPIO ESP-IDF
6.1 (Apache-2.0). Sprawdzono lokalne nagłówki `spi_master.h`, `spi_common.h`,
`gpio.h` oraz API pamięci i RTOS. Brak nowych bibliotek zewnętrznych,
Arduino, skopiowanych sterowników ani fragmentów kodu sprzedawcy.

Host SPI2/SPI3 i wszystkie GPIO pochodzą z menuconfig. Wcześniej aplikacja
wykonuje walidację całej mapy przez `board_config`. Sterownik posiada własny
bus, jeden właściciel zadaniowy, transmisje synchroniczne. Alokuje przy open
wewnętrzny bufor DMA 8192 B; present kopiuje do niego framebuffer z RAM/PSRAM.
Małe komendy korzystają z tx_data. Nie alokuje w present; nie wywoływać go
ze ścieżki PCM/I²S. Zasoby są zwalniane przy błędach otwierania oraz close.

SPI mode 0, MSB first, maksimum 10 MHz według minimalnego tcycle=100 ns;
usunięto wcześniejsze dopuszczenie 40 MHz z Kconfig. Zachowano domyślne
10 MHz i GPIO 11/12/9/10. Dodano cykl setup/hold CS i zwłokę co najmniej
300 ms przed komendami. Opcjonalny reset GPIO jest aktywny LOW przez >=1 ms;
dla profilu PCB nie jest wykonywany. Po AF odczekuje >=200 ms.

## Profil do eksperymentu, nie wynik pomiarów

- Dual COM, MUX=64, horizontal increment.
- Offset=28, remap=0x14 — hipoteza wyśrodkowanego okna i orientacji; dostępne
  przez menuconfig. Wymagana akceptacja obrazu i obu krawędzi.
- Wewnętrzny regulator VDD, zewnętrzne VSL zgodnie ze schematem modułu
  (sieć rezystor/dioda na VSL). Timingi, precharge, VCOMH i enhancement normal
  ustawione jawnie według reset values z dokumentacji kontrolera.
- C1=0x7f (reset value), C7=3 (4/16 prądu) to ograniczony prąd testowy;
  jasność docelowa i parametry panelu wymagają pomiarów.
- Domyślna tabela B9, 16 kodów pikseli. Nie jest to zmierzona gamma;
  GS0/GS1 mogą być wizualnie zbliżone według tabeli kontrolera.

Oddzielne zadanie diagnostyczne pokazuje czerń, biel, wzór i jego negatyw
po 3 sekundy, następnie wyłącza panel. Loguje czas pełnego present (obejmuje
kopię i komendy), bez deklarowania zmierzonego FPS przed testem PCB.
`STREAMOO_LCD_SMOKE_TEST=y` w tym etapie, można wyłączyć w menuconfig.
Framebuffer testu w PSRAM, staging DMA wewnętrzny. To nie jest renderer UI.

## Weryfikacja i brama

Testy hostowe wykonują prawdziwy kod C z rejestrującym transportem i błędami
w każdym miejscu init/present, sprawdzają D/C, adresy, rozmiary i granice 4bpp.
Podgląd PNG generuje `tools/preview_ssd1322.py` z tej samej funkcji firmware.
SPI jest write-only, bez ACK/odczytu; ESP_OK nie potwierdza reakcji panelu.
Build nie dowodzi elektrycznej poprawności resetu, zasilania, VSL ani obrazu.
Przed uznaniem 04a za odebrany wymagany test z `HARDWARE_BRINGUP.md`.
Pełny etap 04 (renderer, fonty, UI) pozostaje niezrealizowany.

## Źródła

- [Moduł wskazany przez użytkownika](https://sklep.msalamon.pl/produkt/wyswietlacz-oled-312-256x64px-zolty/).
- [Schemat/pinout modułu](https://sklep.msalamon.pl/wp-content/uploads/2023/01/sklep_msalamon_OLED_312_XXX_pinout.jpg).
- [Poradnik ze sprzecznymi podpisami](https://sklep.msalamon.pl/blog/modyfikacja-wyswietlacza-oled-3-12%E2%80%B3-ssd1322-z-interfejsu-8080-na-6800-3spi-4spi/).
- [Zdjęcie z oznaczeniami R5(0)/R6(1), R8(0)/R7(1)](https://sklep.msalamon.pl/wp-content/uploads/2024/03/sklep_msalamon_OLED_312_art5.jpg).
- [SSD1322 rev.1.2](https://sklep.msalamon.pl/download/30674/?tmstv=1675090337),
  tabele 7-2, 8-1, 9-1 i 13-4, sekcje 8.9 i 10.
