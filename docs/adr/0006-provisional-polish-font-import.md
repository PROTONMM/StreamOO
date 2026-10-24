# ADR 0006 — import bitmap PL na wyraźne polecenie użytkownika

Data: 2026-10-24. Status: import zatwierdzony przez użytkownika;
licencja modyfikacji nadal OPEN / DO SPRAWDZENIA.

## Nowa decyzja

Po analizie ADR 0005 użytkownik polecił: „Nie mam linku, poszukam, na razie
skopiuj i odnotuj w dokuementacji do sprawdzenia.” To nowsza, bezpośrednia
instrukcja dotycząca tego konkretnego zasobu. Zastępuje wcześniejsze
wstrzymanie importu z powodu braku danych licencyjnych. Nie oznacza ustalenia
licencji od autora, przyjęcia GPL przez projekt ani przypisania BSD do zmian.
Nie żądano ponownego potwierdzenia tej samej decyzji.

## Zakres importu

`tools/import_polish_font.py` używa sprawdzonego parsera initializer danych;
wymaga dokładnego SHA-256 źródła z ADR 0005 i generuje natywny plik C.
Dołączono jedynie ASCII U+0020..007E (95 znaków) i 18 polskich liter z jawnego
mapowania Unicode. Same bitmapy mają 565 B; struktura lookup ma dodatkowe
pola kodów Unicode i wyrównanie. Pozostałych znaków, ikon i kodu Arduino
ani `utf8Rus.cpp` nie przeniesiono. Osiem bitów każdej kolumny zachowano.
Lookup nie transliteruje Unicode na stare bajty; nieznany znak zgłasza false
bez zapisu wyjścia, a renderer stosuje własny box fallback.

`THIRD_PARTY_NOTICES.md` zawiera pochodzenie, hash źródła, zakres danych,
wyraźne polecenie użytkownika i nierozstrzygnięty status polskich zmian.
Dołączono notę BSD bazowego Adafruit GFX w `licenses/Adafruit-GFX-BSD.txt`.
Nie przedstawiono jej jako potwierdzonej licencji modyfikacji.
Wynik dalszego sprawdzenia licencji należy dopisać po znalezieniu źródła.

## Integracja i testy

Backend `streamoo_font5x8` jest oddzielony od dekodera UTF-8 i sterownika.
Rysowanie nadal przechodzi przez canvas 4bpp i clipping. Brak alokacji w
lookup/rysowaniu oraz nowych zależności runtime.

Aplikacyjny `text_smoke` jest szóstym obrazem (`step=5`): małe/duże polskie
litery, pangram i ogonki na ostatnim wierszu. Poprzednie obrazy pozostają,
całość trwa około 18 s, następnie panel gaśnie. Test nie udaje docelowego UI,
fontu Krypton ani marquee. Nie wykonano automatycznego flashowania.

Cztery nowe testy sprawdzają digest wszystkich 113 bitmap względem źródła,
rozdzielenie Unicode od starych indeksów, ósmy bit ogonków, pełną klatkę
wzorcową i canary. Wzorzec PNG obejrzano przed zamrożeniem framebufferu.
W repo testy działają bez prywatnego ZIP; importer wymaga go tylko do
odtworzenia danych. Podgląd hostowy nie zalicza testu napisów na OLED.

Następne prace: odbiór polskich znaków na panelu, następnie marquee,
wyrównanie tekstów i integracja większego fontu Krypton. D14 w zakresie
licencji pozostaje OPEN mimo wykonanego importu.
