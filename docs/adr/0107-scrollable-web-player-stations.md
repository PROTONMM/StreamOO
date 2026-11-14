# ADR0107 — wszystkie stacje w przewijanym oknie odtwarzacza

Data:2026-11-14. Status:zaimplementowane na polecenie użytkownika;
odbiór wyglądu i przewijania w rzeczywistej przeglądarce OPEN.

## Problem i zmiana

Użytkownik chce w odtwarzaczu WWW jedno okno z całą listą, bez podziału
na strony. Usunięto stationPage, przyciski poprzednie/następne i slice50.
Lista renderuje wszystkie wyniki wyszukiwania, z wysokością ograniczoną
do min(60vh,32rem), overflow-y:auto i stabilnym miejscem na scrollbar.
Dodano region z nazwą i tabindex, focus-visible oraz zawijanie długich nazw.
Przeglądarka obsługuje scroll myszą/dotykiem/klawiaturą natywnie.

Filtrowanie resetuje scroll na początek. Odświeżenie zachowuje pozycję
w granicach nowej zawartości. Zmiana aktywnej stacji aktualizuje aria-pressed
istniejących przycisków i nazwę stacji, bez rekonstrukcji listy lub utraty
focus/scroll. Przyciski mają data-station-id i bezpieczne textContent.
Licznik pokazuje ilość wszystkich stacji albo wyniki/wszystkie.

Zmiana dotyczy tylko odtwarzacza. Paginacja edytora pozostaje25 wierszy.
Brak zmian REST, limitu2048 stacji, generacji/CAS, katalogu/NVS/trybu AP,
sterowania audio lub importu. Lista powstaje w przeglądarce; serwer nadal
zwraca ten sam katalog. Nie dodano frameworka ani zależności.

## Weryfikacja

-17 istniejących testów test_portal*.py PASS.
- node --check components/portal/web/portal.js PASS.
- Ręczny harness DOM w Node:2048 stacji obecne jednocześnie, wybór ostatniej
przekazuje ID i generację, filtrowanie i licznik, puste wyniki/katalog,
zaznaczenie oraz zachowanie scroll po statusie/odświeżeniu PASS.
To walidacja logiki DOM z atrapą, nie rzeczywisty test layoutu przeglądarki.
- ESP-IDF6.1 build PASS,0x173750B,90% app wolne.
- git diff --check PASS.

Agent nie flashował ani nie publikował panelu. Do odbioru po flash:otworzyć
Odtwarzacz, przewinąć do końca, wyszukać stację, wybrać ją i potwierdzić
brak skoku listy po odświeżeniu statusu; także ekran telefonu i klawiatura.
