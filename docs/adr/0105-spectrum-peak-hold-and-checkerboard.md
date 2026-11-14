# ADR0105 — wspólne widmo maksimum, peak hold i szachownica

Data:2026-11-14. Status:zaimplementowane na polecenie użytkownika;
odbiór wyglądu na OLED i parametrów hold/release OPEN.
Zastępuje spektralne RMS z ADR0103 i rozdział skal ADR0104.

## Decyzja użytkownika

Porzucić RMS, oba widma mają liczyć maksimum prążków pasma i mieć skalę
−45..0dBFS. Drugie ma mieć powoli opadający peak hold; słupki mają być
szachownicą pikseli. Zachowujemy gamma2 z ostatniej zaakceptowanej skali.
RMS/peak stereo bargraphu i wskazówek nie są przedmiotem usunięcia.

## Implementacja

Tryb0:24 pasma maksimum, szachownica1x1 GS10/czerń. Tryb3:te same słupki
plus niezależne jasne kreski GS15, pełna szerokość danego słupka.
Parzystość szachownicy zakotwiczona do widgetu, bez przesuwania tekstury
podczas zmiany poziomu. Oba dzielą FFT, skalę−45..0dBFS/gamma2 i envelope
słupków320ms. Zwykłe widmo nie rysuje dawnych ciągłych białych wierzchołków.

Parametry początkowe:hold500ms po nowym/równym niezerowym szczycie, następnie
liniowy spadek pełnej skali w2000ms. Nowy wyższy słupek natychmiast podnosi
marker i odnawia hold. Marker nigdy poniżej słupka; pasma mają niezależne
terminy. Obliczenia wyłącznie na nowych ważnych FFT z monotonicznym czasem.
Cofnięty czas nie zmienia stanu. Po długiej luce spadek uwzględnia rzeczywisty
czas; nie odświeżamy timestampów bez danych. Reset i expiry zerują oba
zbiory. Krótkie luki UI mają dotychczasowy ograniczony mostek; w nim szczyty
opadają2s/full-scale, bez fikcyjnego odnawiania hold i pomiaru.

Usunięto obliczanie rms_bands z rdzenia i drugą funkcję skali. Snapshot
meteringu używa dotychczasowego dodatkowego24-elementowego pola na peak
zamiast RMS; model renderera ma jawne24 znaczniki. Dla ogólnego renderera
z maksymalnie64 pasmami szczyty są dostępne dla pierwszych24 — produkcyjny
FFT ma dokładnie24. Model powiększa się o48B, nie o128B. Zapas stosu display
wymaga ponownego pomiaru; nie zmniejszono stosów. Jedna struktura envelope
zastępuje dwie poprzednie i przechowuje terminy hold. Brak dodatkowej FFT,
nowych zależności, zmian callbacku I2S/PCM/dekodera/NVS/partycji.

Identyfikatory0/1/2 pozostają;3 oznacza teraz SPECTRUM_HOLD. Istniejący
zapis3 automatycznie wybiera nowe widmo, bez migracji lub erase NVS.

## Weryfikacja i odbiór

490 testów hostowych PASS (bez skipów), ASan/UBSan:envelope,10 metering,
15 controls i stress renderera4 trybów PASS; LeakSanitizer off.
Walidator platform/GPIO i git diff --check PASS. Native ESP-IDF6.1 build
PASS, obraz0x173660B,90% partycji aplikacji wolne. Zmienione klatki
HOME obejrzano jako PNG przed zamrożeniem. Testy sprawdzają pełną skalę,
checkerboard, clipping, identyczne korpusy, osobny marker nawet przy zerowym
słupku, walidację markerów oraz hold/opadanie/reset/ciszę/niezależne pasma.
Hostowe testy nie potwierdzają wyglądu i zapasu stosu na fizycznym OLED.

Nie flashowano urządzenia. Odbiór:porównać tryby0 i3 na tej samej audycji;
słupki mają mieć te same wysokości, wyłącznie tryb3 ma wolno opadające kreski.
Sprawdzić ciszę, STOP/PLAY, zmianę stacji oraz zapis trybu po5s i reboot.
