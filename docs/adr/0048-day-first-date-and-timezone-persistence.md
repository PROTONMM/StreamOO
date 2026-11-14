# ADR0048 — dzień/miesiąc/rok i trwałość strefy

Data:2026-10-31. Użytkownik potwierdził zegar i zmianę strefy na sprzęcie,
wymaga kolejności dzień–miesiąc–rok oraz pamiętania strefy w NVM.

Renderer daty zmieniono na DD.MM.YYYY, np.31.10.2026. Nieznany czas
--.--.----. Geometria, godzina HH:MM:SS i strefy/DST pozostają bez zmian.
Aktualizacja4 syntetycznych golden frames po obejrzeniu podglądu;
sprawdzono, że różnice ograniczone do panelu daty x176..255/y32..39.

Strefa już zapisywana w NVS streamoo_clock/zone z commit i odczytem przy
starcie (ADR0047), bez zmian produkcyjnego ownera. Dodano test: pierwszy
proces zapisuje Europe/London do trwałej atrapy NVS w pliku, drugi świeży
proces odczytuje ją bez ponownego zapisu. PASS także ASan/UBSan (LSan off).
To potwierdza ścieżkę odtworzenia w kodzie, nie fizyczny power-cycle PCB.
Zegar/zmiana strefy odebrane przez użytkownika; reboot/power-cycle z
wybraną strefą i nowy format daty na OLED pozostają do odbioru.

Bez nowych zależności, zmian audio/partycji i flash/erase/eFuse.

Weryfikacja:329 testów hostowych PASS, clock-settings ASan/UBSan PASS
(LSan off), native ESP-IDF6.1 build PASS,0x160ff0 (6% wolne),
check_config --platform i diff --check PASS. Bez flash/erase/eFuse.
