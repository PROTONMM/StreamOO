# ADR 0008 — font nagłówka i renderer maski 4bpp

Data: 2026-10-24. Status: 04e programowo zweryfikowane;
font i przewijanie na OLED HARDWARE_TEST_PENDING.

## Źródło i zgodność

Oficjalny Monaspace v1.400, statyczny MonaspaceKrypton-Regular.otf; źródło
wersjonowane w assets/fonts, hash i link do wydania w tamtejszym README.
Licencja OFL-1.1 została sprawdzona i zachowana z copyright i Reserved Font
Names. Font po konwersji ma nazwę StreamOO UI16; Krypton jest nazwą źródła
w atrybucji, nie nazwą zmodyfikowanego podzbioru. Nie zmienia to otwartej
kwestii licencji oddzielnego małego fontu PL.

Nowe API runtime to własny C11 wykorzystujący graphics/text i standardowe
nagłówki C ESP-IDF 6.1; brak nowych bibliotek runtime, Arduino ani LVGL.
Generator używa zainstalowanych Pillow 10.2.0, fontTools 4.46.0 i FreeType
2.13.2 (manifest). Sprawdzono API getbbox/getmetrics/getlength i cmap na
rzeczywistym pliku; nie kopiowano implementacji tych bibliotek.
Build ESP-IDF nie wymaga uruchamiania generatora.

## Format i działanie

Wybrano rozmiar rasteryzacji 13 px: ascent=13 i descent=3. Sprawdzone bbox
wszystkich 113 glifów mieszczą się w 16 px. Nie przycięto akcentów i ogonków
przez założenie, że font o nominalnym rozmiarze 16 musi zmieścić się w 16px.
Podzbiór: ASCII 32..126 i polskie 18 liter, 4448 B bitmap plus metryki.

Naprawiono narzędzie: każdy wiersz 4bpp ma ceil(width/2) bajtów, padding
nieparzystego glifu jest zerowy; glify nie dzielą nibble między wierszami.
Generator sprawdza cmap i metryki, nie zastępuje po cichu brakujących znaków.
Zapisuje pochodzenie, rozmiar, bearing, advance, offset i liczbę bajtów.

Renderer fontu ma sorted Unicode lookup i fallback ?. Przed zapisem waliduje
metryki wszystkich użytych glifów i ich zakresy w bitmapie. Statyczny opis
fontu, jego długości i uporządkowanie tabeli są zaufanymi danymi z generatora;
nie jest to parser dowolnych plików fontów przesyłanych przez sieć.
Baseline, advance i clipping sterują pozycją. Text input nie może nakładać się
na framebuffer. Tekst i font są niemodyfikowane; brak alokacji, float i RTOS.

Graphics otrzymał alpha4: coverage 0..15 komponowane z kolorem i tłem przez
`(alpha*foreground + (15-alpha)*background + 7)/15`. Alpha=0 nie zmienia
pikseli. Zapis respektuje granice, clip, padding i drugi nibble bajtu.
Przewijanie fontu używa zegara i parametrów z ADR 0007.

## Integracja i odbiór

W ośmiosekundowej scenie animacji nagłówek „StreamOO - Zażółć gęślą jaźń”
używa teraz StreamOO UI16, centrowany w bounds 256×16. Pozostałe wiersze
pozostają bitmapowe 5×8 i przewijane. Cała diagnostyka nadal około 26 s,
potem OFF. To test fontów/layoutu, nie radio ani model stacji.

Testy obejmują wszystkie zakresy 113 glifów i ich linię 16px, Unicode,
fallback, alpha i padding, błędny span, pusty zły font, cały framebuffer,
nieparzyste wiersze generatora, cmap i odrzucenie niepoprawnych skalarów.
Zamrożony wzór sceny obejrzano na hoście; test go nie regeneruje.
ASan/UBSan sprawdziły pozycje i clipping na siatce bounds. LeakSanitizer jest
niedostępny pod ptrace, użyto detect_leaks=0; nie raportujemy testu wycieków.

Na urządzeniu należy ocenić duży font, polskie akcenty, separację y15/y16,
centrowanie, kontrast antyaliasingu i ruch. Nie zadeklarowano tych testów jako
zaliczonych. Dalszy UI/widgety wymagają potwierdzenia czytelności tego układu.
