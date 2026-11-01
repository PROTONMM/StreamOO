# ADR0087 — timingi panelu zamiast wartości resetowych SSD1322

Data:2026-11-01. Status: protokół host PASS, wynik obrazu na module OPEN.

Użytkownik: przygasa FLAC1500k tylko na wysokości bargraphu, reszta obrazu
OK; ma oscyloskop, ale wskazuje poprawne działanie tego sprzętu na innym
firmware. Logo i wskazówki stabilne. Szachownica nie usunęła objawu.
Nie uznajemy globalnego zasilania za główną hipotezę bez dodatkowych danych.

W rendererze FLAC y32..39 ma GS12, bitrate y40..47 GS10. Bargraph L zajmuje
y33..46. To wspólne wiersze skanowania. Nowy test pełnego HOME sprawdził
identyczność bajtów napisu przy zmianach poziomu i rodzaju widżetu;
nie ma programowego przygaszania, wymazywania ani rysowania bargraphu poza
przydzielonym bounds. Hostowy test nie mierzy sygnałów ani optyki panelu.

## Porównanie i decyzja

Pierwszy bring-up ADR0003 używał timingów resetowych kontrolera, bez
potwierdzenia jakości gęstych półtonów. Referencja256×64 ma inne ustawienia:

| Rejestr | Dotychczas | Nowa próba |
|---|---|---|
| B3 zegar skanowania |0x50: oscillator5, divider1|0x91: oscillator9, divider2|
| B1 reset/precharge |0x74:9/7 DCLK|0xE2:5/14 DCLK|

Te dwie wartości podaje przykład panelu256×64 NHD-3.12-25664UCY2,
[dokument producenta, s.9](https://newhavendisplay.com/content/specs/NHD-3.12-25664UCY2.pdf).
To panel referencyjny z tym samym kontrolerem, nie potwierdzenie identyczności
z modułem msalamon. Zmiana jest profilem do odbioru na sprzęcie użytkownika.
Znaczenie pól sprawdzone w
[SSD1322 rev.1.2, tabela9-1 i sekcja10.1.14/15](https://newhavendisplay.com/content/app_notes/SSD1322.pdf).

Również U8g2 ma tę parę w inicjalizacji256×64; przeczytano źródło
i licencję BSD-2-Clause. Nie importowano biblioteki, makr ani kodu sterownika.
Własna sekwencja zmienia tylko dwa parametry rejestrów opisane powyżej.
[Źródło referencyjne](https://github.com/olikraus/u8g2/blob/master/csrc/u8x8_d_ssd1322.c),
[licencja](https://github.com/olikraus/u8g2/blob/master/LICENSE).

Nie przenosimy całego profilu obcego modułu: C1=0x7F, C7=3, BB=0x17,
BE=4, VSL/enhancement, tabela B9 i remap pozostają. Nie zwiększamy prądu
ani napięć. Szachownica oraz FPS UI25/SPI10MHz też pozostają.
Nie deklarujemy rzeczywistego80Hz z komentarza przykładu producenta;
częstotliwość skanowania tego modułu nie została zmierzona.

## Odbiór

Test protokołu sprawdza dokładne dane B3/B1 i poprawne D/C;9 testów PASS.
Build i pełne regresje według STATUS. Agent nie flashował.
Po flash wybrać ten sam bargraph i obserwować FLAC1500k przy małej/dużej
wartości słupków; sprawdzić też widmo i wskazówki. Podać, czy zniknęło
przygasanie oraz czy zmieniła się czytelność półtonów. Nie zaliczać po buildzie.
W razie dalszego objawu badać kolejno pozostałe parametry panelu i stały wzór
HOLD, zamiast zmieniać równocześnie prąd, napięcia i gamma. FLAC/audio ma
osobną poprawkę ADR0086; brak podstaw do łączenia obu przyczyn w jedną.
