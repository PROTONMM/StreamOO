# 05 — Enkoder, gesty, lista stacji

**USTALONE:** jeden enkoder z przyciskiem, dwa warianty A/B wybierane przez menuconfig.

| Gest | A (domyślny) | B |
|---|---|---|
| Obrót w ekranie głównym | głośność | głośność |
| Krótki klik | lista stacji | play/stop |
| Długi klik | play/stop | lista stacji |
| Wciśnięcie + obrót | zmień FFT/bargraph/analog | zmień FFT/bargraph/analog |
| Obrót na liście stacji | płynne przewijanie | jw. |
| Krótki klik na liście | zatwierdź środkową | jw. |
| Długi klik na liście | wyjdź bez zmiany | jw. |

**PROPONOWANE**: debounce 20ms, długi ≥600ms, akcja długiego przy zwolnieniu; po obrocie z przyciskiem zwolnienie nie emituje kliknięcia. Definiuj gest jako abstrakcyjne zdarzenia `ROTATE`, `SHORT`, `LONG`, `PRESSED_ROTATE` z delta.

## Przewijanie
- Pozycja logiczna selection-index niezależna od pozycji animowanej `position_float`. Zwiększaj cel o detent, interpoluj render bez gubienia impulsów.
- Widoczna centralna stacja jaśniejsza, dalsze przygaszone; lista przewija się góra/dół, wrap-around do określenia na podstawie UX testów (**OTWARTE**).
- Dopóki użytkownik nie zatwierdzi stacji, nie ruszaj strumienia audio.
- Po zatwierdzeniu zainicjuj `SelectStation(stable_id)` i wróć do głównego ekranu natychmiast; nie blokuj UI.

## Trwałość
Zmiany głośności i wizualizacji: debounce zapisu NVS 5 sekund od **ostatniej** zmiany, tylko przy różnicy; kontrolowany shutdown/restart flush. Obrót przy naciśniętym przycisku zmienia wizualizację bez regulacji głośności.

## Przygotowany interfejs UI

Etap 04h dostarcza `streamoo_ui_open_stations/move/confirm/cancel`.
Przyszły adapter gestów ma wywoływać je w jednym zadaniu UI; callback GPIO
nie może rysować ani mutować UI bezpośrednio. Kursor i animacja są oddzielone
(Q16 zamiast float). Sam ten interfejs nie jest implementacją enkodera,
debounce ani mapowania A/B. W teście granice są clamped; finalny wrap to D19.

## Część 05a — dekoder i charakterystyka sprzętu

Komponent `input` implementuje przejścia A/B oraz abstrakcyjne gesty.
[ADR 0013](adr/0013-encoder-decoder-characterization.md) opisuje semantykę.
Domyślnie włączony `STREAMOO_ENCODER_GESTURE_TEST` wypisuje w konsoli TEST
ROTATE/SHORT/LONG/PRESSED_ROTATE i delta. Obecny adapter próbkuje co tick,
więc należy testować wolne ruchy; szybkie mogą gubić zbocza. Nie steruje UI.
Półkrok, kierunek, pull-up i timing wymagają odbioru. Etap 05 pozostaje częściowy.
