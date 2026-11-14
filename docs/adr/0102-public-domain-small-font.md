# ADR0102 — public-domain Misc Fixed Unicode 5x8

Data:2026-11-14. Status:przyjęte i zaimplementowane na polecenie użytkownika.
Zastępuje tymczasowy wybór bitmap ADR0006 dla bieżącego firmware.

## Powód

Mały font miał nieustalone prawa do polskich modyfikacji z prywatnego ZIP.
Użytkownik polecił wymianę po sprawdzeniu alternatyw. Wybór: Misc Fixed
Unicode5x8 z public-domain deklaracją w BDF i potwierdzeniem rozszerzeń
Unicode na stronie opiekuna. Oryginalni autorzy ASCII/Latin-1 nie są znani;
zachowujemy dokładną deklarację upstream, nie nadajemy zasobowi MIT.

## Rozwiązanie

Źródło `assets/fonts/misc-fixed/5x8.bdf`, rev1.32/2006-01-05, SHA-256
`a126e263c6ee843e8725fa3ad630879e74d3294493ce7a00f9a9a21ae10fc2f3`.
Pochodzenie/warunki: `licenses/Misc-Fixed-NOTICE.md`. Generator
`tools/font_generator/bdf_to_font5x8.py` sprawdza hash, komplet113 glifów,
5x8/offset0,-1 i zerowy padding; transponuje wiersze MSB-left na kolumny
bit0-top bez skalowania/przycinania. Normalny build korzysta z gotowego C;
nie wymaga BDF parsera ani nowych zależności ESP-IDF.

API providera i komórka6x8 pozostają; duży Krypton/renderer/layout/DSP/audio
bez zmian. Stary importer ZIP usunięty, inspector zachowany wyłącznie jako
narzędzie historycznej analizy. Sześć klatek z małym fontem odtworzono i
obejrzano jako PNG przed zamrożeniem. Bieżące klatki nie zawierają starego
fontu PL. Historia Git i materiały referencyjne zachowują wcześniejszy status.
Noty i raporty licencji otrzymały datowane uzupełnienie; ta zmiana nie nadaje
MIT własnej aplikacji ani nie zmienia warunków pozostałych zależności.

## Weryfikacja

- `python3 -m unittest discover -s tests -q`:486 testów PASS, bez skipów.
- Nowy niezależny oracle pikseli BDF oraz odtwarzalność generatora PASS;
  istniejące testy Unicode/ogonka/granic i klatek UI PASS.
- `tests/bitmap_font_sanitizer.c` z ASan/UBSan PASS;
  `ASAN_OPTIONS=detect_leaks=0`, bez deklaracji testu wycieków.
- `python3 tools/check_config.py sdkconfig --platform`:PASS.
- ESP-IDF6.1 `idf.py build`:PASS,0x173480B,90% app wolne.
  Użyto systemowego `/usr/bin/cmake` zamiast niedziałającego Snap CMake.
- `git diff --check`:PASS.

Nie flashowano urządzenia. Odbiór wyglądu na OLED OPEN: polskie litery,
scroll tytułu, IP/VOL, bitrate i data. Hostowe PNG nie zastępują OLED.

## Odbiór fontu na OLED — ADR0102 / 2026-11-14

Użytkownik wgrał firmware i zaakceptował nową czcionkę: jest drobniejsza
oraz mniej czytelna od poprzedniej, ale może pozostać. Odbiór wyglądu
na OLED zaakceptowany z tym zastrzeżeniem; bez kolejnej zmiany bitmap.
Nie podano identyfikatora firmware ani szczegółowych wyników dla każdego
polskiego znaku, scrolla i poszczególnych pól — nie zaliczamy tych prób
osobno na podstawie ogólnej akceptacji. Bez zmian kodu, builda i flash agenta;
weryfikacja tej aktualizacji dokumentacji: `git diff --check` PASS.

