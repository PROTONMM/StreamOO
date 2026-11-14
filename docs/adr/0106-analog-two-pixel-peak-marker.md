# ADR0106 — dwupikselowy peak mierników wskazówkowych

Data:2026-11-14. Status:zaimplementowane na polecenie użytkownika;
odbiór czytelności OLED OPEN.

Pojedynczy punkt peak był słabo widoczny. Dla obu kanałów rysujemy
piksel GS15 na dotychczasowej pozycji łuku (x,y) i drugi (x,y+1).
Clipping przez canvas zachowany. Bez zmian położenia, skali, RMS/peak,
ballistyki, widm, I2S i pozostałych rendererów.

Weryfikacja:50 istniejących testów visualization/home/ui/controls PASS,
ręczna kontrola obu pikseli w obu kanałach PASS, ASan/UBSan stress renderera
PASS (LSan off), ESP-IDF6.1 build PASS (0x173680B,90% app wolne), diff check
PASS. Nie flashowano; ocenić widoczność po wgraniu na OLED.
