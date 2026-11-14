# ADR0053 — użyteczniejszy początek skali głośności

Data:2026-10-31. Status: wdrożone, charakterystyka odebrana odsłuchowo przez użytkownika.

Użytkownik zgłasza, że do około1/3 skali prawie nic nie słychać i chce
bardziej liniowej charakterystyki. Dotychczasowa regulacja miała liniowy
przebieg w dB od-60 do0, co przy33 dawało około-40dB.

Zmieniamy wyłącznie mapowanie VOL0..100 na amplitudę:
`gain = (VOL/100)^2`, tablica Q16 wyliczona offline i zaokrąglona do
najbliższej liczby całkowitej. Krzywa pośrednia między dotychczasową
eksponencjalną a liniową amplitudą. Nie deklarujemy liniowej odczuwanej
głośności: to zależy od wzmacniacza, kolumn, nagrania i warunków odsłuchu.

| VOL | Poprzednie tłumienie | Nowe tłumienie (przed kwantyzacją Q16) |
|---|---|---|
| 10 | -54dB | -40dB |
| 33 | -40,2dB | -19,3dB |
| 50 | -30dB | -12,0dB |
| 75 | -15dB | -5,0dB |
| 100 | 0dB | 0dB |

0 nadal dokładnie mute,100 unity; bez wzmocnienia ponad unity. Krzywa
ściśle rosnąca dla wszystkich101 pozycji. Wspólna dla WWW i enkodera,
bez zmiany zapisanej liczby VOL w NVS. Ten sam zapisany poziom poniżej100
będzie teraz głośniejszy. Rampa50ms, stereo i integer PCM bez zmian;
callback nie dostaje obliczeń float/libm ani alokacji. Wskaźniki mierzące
przed gain zachowują wcześniejsze znaczenie.

Odbiór: po zwykłym flash rozpocząć od małej głośności, ocenić zakres
10/20/33/50 oraz płynność regulacji enkoderem i WWW. Nie zmieniać NVS,
partycji ani FFT przy tym teście. Dotychczasowa specyfikacja krzywej
w ADR0019 pozostaje opisem historycznego etapu, zastąpionym tą decyzją.

## Odbiór — 2026-10-31

Użytkownik potwierdził: „charakterystyka głośności jest teraz dobra”.
Przyjęto obecną krzywą kwadratową; dalsze strojenie nie jest potrzebne.
