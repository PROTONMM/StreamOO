# ADR0108 — wspólne logo StreamOO dla OLED i WWW

Data:2026-11-14. Status:wdrożone; ocena na fizycznym OLED OPEN.

Użytkownik prosi o ładniejsze logo. Przyjęto wspólny znak dla ekranu
startowego i panelu WWW: większy autorski napis Stream i połączone OO
przypominające nieskończony strumień. Zastępuje grafikę ADR0063.
Zaokrąglone kreski o stałej grubości, wygładzone brzegi, bez dodatkowej
ikony po lewej i linii sygnału. Podpis INTERNET RADIO rozstrzelony,
jasność7/15; główny napis15/15. WWW: grafit i zieleń obecnej palety.

Źródłem są własne ścieżki M/L/C w tools/generate_brand_mark.py, bez
importowanych obrysów fontów, ikon lub kodu stron trzecich. Generator
hostowy pycairo1.25.1 zapisuje SVG i maskę4bpp224×40, aktualizuje też
inline SVG w HTML. Rasteryzacja4× i redukcja do16 poziomów zachodzi
na hoście. Cairo nie jest zależnością firmware ani WWW. Podpis OLED
używa dotychczasowego Misc Fixed/public domain; istniejące noty pozostają.

Maska4480B const w flash, render przez istniejące canvas_alpha4 do
istniejącego framebuffer8192B; brak alokacji, drugiego framebuffer
lub dużych tablic na stosie. Timing/polityka boot z ADR0068 oraz audio
pozostają bez zmian. W panelu SVG jest lokalnie osadzony, z nazwą dostępną
aria-label StreamOO, skalowany do szerokości nagłówka bez nowego endpointu,
fontów z sieci ani zmian CSP. Generator i podglądy opisane w assets/branding.

Walidacja:
- 37 istniejących testów boot_transition/canvas/ui/home PASS.
- 17 istniejących testów portalu PASS; node --check PASS.
- Rzeczywisty renderer hostowy: -Wall/-Wextra/-Werror, błędny rozmiar
  bez zapisu, guard bytes po rysowaniu PASS; podgląd4× obejrzany.
- Ponowna generacja SVG/maski/inline SVG identyczna bajtowo PASS.
- Native ESP-IDF6.1/esp32s3 build PASS:0x174a10B,90% partycji app wolne.
- Walidator platform/GPIO i git diff --check PASS.

Bez flash agenta. OPEN:jasność/ostrość na fizycznym SSD1322 i wygląd
nagłówka w przeglądarce urządzenia. Podgląd PNG to rzeczywisty framebuffer,
nie zdjęcie wyświetlacza; testy portalu nie zastępują wizualnej oceny WWW.
