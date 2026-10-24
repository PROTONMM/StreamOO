# ADR 0005 — identyfikacja czcionki PL i niezależna warstwa UTF-8

Data: 2026-10-24. Status: baza 04c-1 przyjęta; import bitmap OPEN do wyjaśnienia
licencji. Nie oznacza ukończenia fontów ani UI.

## Materiał użytkownika

Użytkownik wskazał lokalny ZIP z polską modyfikacją yoRadio jako źródło znaków.
Sprawdzono cztery pliki: `glcdfont.c`, `utf8Rus.cpp`, `displayL10n_custom.h`
i `Opis.txt`. Instrukcje podmiany bibliotek Arduino w archiwum są materiałem
referencyjnym, nie poleceniem dla StreamOO. Nie wykonano ich i nie dodano
Arduino, konwertera źródłowego ani bitmap do firmware/repozytorium.

`glcdfont.c` SHA-256:
`b837c600bebad57c18de4f5f6b2f1da6874a206a3b225d4b63e0e73e6729c7da`.
256 glifów po 5 bajtów (1280 B); bajt oznacza kolumnę, bit0 jest u góry.
Choć plik nazywa font 5×7, korzysta z 8. wiersza m.in. dla ą/Ą/ę/Ę.
Przyjmujemy komórkę 6×8: 5 kolumn obrazu i 1 kolumnę odstępu.

Zweryfikowane mapowanie referencyjne (nie używać indeksu jako Unicode):

| Mała litera / Unicode | Slot | Wielka litera / Unicode | Slot |
|---|---|---|---|
| ą U+0105 | B8 | Ą U+0104 | B7 |
| ć U+0107 | BD | Ć U+0106 | C4 |
| ę U+0119 | D6 | Ę U+0118 | D7 |
| ł U+0142 | CF | Ł U+0141 | D0 |
| ń U+0144 | C0 | Ń U+0143 | C1 |
| ó U+00F3 | BE | Ó U+00D3 | BF |
| ś U+015B | CB | Ś U+015A | CC |
| ź U+017A | BB | Ź U+0179 | BC |
| ż U+017C | B9 | Ż U+017B | BA |

`tools/inspect_polish_font.py` czyta wyłącznie initializer danych i tworzy
raport/podgląd. Nie uruchamia kodu ani instrukcji z archiwum. Podgląd powstaje
w ignorowanym build/, nie jest pakowany do firmware. Został obejrzany.

## Licencja — konkretna brakująca informacja

ZIP i nagłówek fontu nie podają licencji ani autora polskich zmian.
Instrukcja wskazuje modyfikację pliku Adafruit GFX. Zweryfikowano oficjalną
[licencję BSD Adafruit GFX](https://raw.githubusercontent.com/adafruit/Adafruit-GFX-Library/master/license.txt)
i [oryginalny glcdfont.c](https://raw.githubusercontent.com/adafruit/Adafruit-GFX-Library/master/glcdfont.c).
Nie wynika z tego automatycznie licencja nieopisanych zmian polskich.
Nie przypisujemy im ani BSD, ani GPL bez dowodu. Zgodnie z zasadą 3 AGENTS.md
nie importujemy zmodyfikowanych bitmap do własnego firmware przed wyjaśnieniem.
Zapytano użytkownika o autora/link/warunki udostępnienia. Nie podjęto decyzji
licencyjnej za użytkownika i nie zmieniono licencji projektu.

## Niezależna implementacja

`text` jest oryginalnym komponentem C11. Zależy wyłącznie od `graphics`
i standardowej biblioteki C dostępnej w zweryfikowanym ESP-IDF 6.1; bez nowych
zewnętrznych bibliotek i bez importu kodu z ZIP. Ścisły dekoder UTF-8 odrzuca
nadmiarowe kodowania, surrogates i wartości >U+10FFFF. Błąd daje U+FFFD,
zużywa jeden bajt i zawsze zapewnia postęp. Bufory mają jawną długość, również
NUL jest daną. Bez statycznego bufora roboczego, alokacji i modyfikacji źródła.

Renderer tekstu obsługuje pojedynczy wiersz 8px, krok 6px i clipping canvas.
Provider przyjmuje rzeczywisty Unicode i zwraca 5 kolumn glifu. Kontrole
renderują się jako spacje; nieznany znak jako własny prosty prostokąt.
Ósmy bit kolumny jest zachowany. Nie ma automatycznego uppercase ani
przepisywania UTF-8 na specjalne bajty yoRadio. Szerokość liczy skalarne znaki,
a dla błędnych sekwencji znaki zastępcze; przepełnienie saturuje SIZE_MAX.
Nie wykonujemy normalizacji Unicode: forma rozłożona wymaga późniejszej
polityki normalizacji lub obsługi znaków łączących.

Testy używają własnego syntetycznego wzoru glifu, nie skopiowanych bitmap.
Backend jest gotowy do podłączenia fontu, lecz aplikacja go jeszcze nie używa
z powodu braku zaakceptowanego zasobu. Nie deklarujemy polskich napisów na OLED,
Krypton, wyrównania/marquee ani pełnej typografii jako zrealizowanych.

## Późniejsza decyzja użytkownika

[ADR 0006](0006-provisional-polish-font-import.md) zastępuje decyzję o wstrzymaniu
importu: użytkownik polecił skopiować bitmapy i odnotować licencję do sprawdzenia.
Status nieznanej licencji nie został tym zmieniony na zweryfikowany.
