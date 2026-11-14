# ADR0083 — kontrolowany test obciążenia i transferów OLED

Data: 2026-11-01. Status: implementacja testu; odbiór panelu OPEN.

Użytkownik potwierdził pulsowanie widoczne gołym okiem, dół w rytm słupków
i sporadyczne przygasanie napisów u góry. Wyjaśniono: czyszczenie canvas
odbywa się w RAM; present wysyła dopiero kompletny obraz, bez blank frame.
Sam objaw nie potwierdza winy FPS, PSU ani konkretnego rejestru SSD1322.

Decyzja: opcjonalny test pięciu faz, każda po 8 s: stały wzór bez kolejnych
transferów, ten sam wzór przesyłany 25 FPS, zmienny obszar świecenia 25/10
FPS, następnie ruch ze stałą liczbą pikseli każdego półtonu. Górny napis
i pola szarości są identyczne przez cały test. Zawartość HOLD/REPEAT
jest bajtowo równa; funkcja animacji LOAD25/LOAD10 jest ta sama.

STREAMOO_LCD_TIMING_PROBE domyślnie n, jeden przebieg po resecie, po PLAYING
w normalnym widocznym ekranie. Dotyczy tylko właściciela OLED; audio/WWW
działają w tle. Pierwszy event enkodera przerywa test i zostaje zużyty.
Log zawiera fazę, liczbę transferów, rzeczywisty czas i avg/max present.
Powrót do UI bez powtarzania logo i restartowania playera. Nie zmieniono
profilu analogowego, SPI, normalnych 25 FPS, buforów, codec ani NVS.

Scena to własny renderer na istniejącym canvas/font; bez nowych bibliotek,
alokacji w pętli ani dostępu do I2S. Testy hostowe sprawdzają wzorce kontrolne,
stałe referencje u góry, liczbę pikseli każdej jasności, zmianę obciążenia,
granice pamięci i odrzucanie błędnych argumentów. Build i wyniki hostowe
odnotowane w STATUS; brak fizycznego pomiaru przez agenta.

Procedura i ograniczenia: [OLED_TIMING_BRINGUP](../OLED_TIMING_BRINGUP.md).
