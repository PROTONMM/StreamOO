# ADR0046 — odstęp RSSI od wizualizacji

Data:2026-10-31. Użytkownik: RSSI np.-69 dotyka wizualizacji.
Przesunięto pole tekstu z x20 na x14 (szerokość24px), pozostawiając
6px pustej przestrzeni przed wizualizacją x44. Ikona Wi-Fi zwężona
do kolumn2px z krokiem3px, x1..11; znak offline również w tym zakresie.
Dzięki temu4 znaki -127 nie nachodzą na ikonę. Reszta geometrii bez zmian.

12 testów HOME i13 controls PASS, native ESP-IDF6.1 build PASS.
Zaktualizowano4 syntetyczne golden frames po obejrzeniu podglądu;
sprawdzono, że różnice ograniczone do x0..43/y51..63. Nie dodano
nowych testów dla tej drobnej zmiany. Odbiór fizycznego OLED OPEN.
Bez zmian audio, flash/erase/eFuse i nowych zależności.
