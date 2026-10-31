# ADR0072 — krótkie luki widma nie migają „Brak danych”

Data: 2026-10-31. Status: zaimplementowane, odbiór HLS/OLED OPEN.

## Dowód

Użytkownik zgłosił krótkie pojawianie się „Brak danych” dla `TEST HLS
64k-heaacv2-stereo`. Log tej samej generacji11 pokazuje ciągłe odtwarzanie:
`PCM=32`, `low=0`, `hard=0`, `starts=1`. To wyklucza underrun jako przyczynę
tego komunikatu.

Jednocześnie tap meteringu jest droppable: rosły `gaps/drop`, a `max_work`
osiągnął 418–656ms. Brak świeżej ramki FFT przez ponad200ms poprawnie ustawia
`spectrum_valid=0`; HOME zamieniał wtedy wykres na „Brak danych”, aż następne
okno FFT było gotowe.

## Decyzja

UI zachowuje ostatni prawidłowy model wizualizacji przez najwyżej800ms, tylko
gdy otrzyma nieważny model tej samej stacji, generation i aktualnego typu
widżetu. Nowsza prawidłowa klatka odnawia licznik. Zmiana stacji/generation,
STOP, zmiana widżetu albo przekroczenie800ms nie zachowują starego modelu i
pozostawiają dotychczasowy komunikat.

To jest wyłącznie stabilizacja renderingu. `spectrum_valid`, RMS/FFT expiry,
liczniki gaps/drop, diagnostyka i droppable tap nie są zmieniane; nie
ukrywamy niedoboru PCM ani nie dotykamy I2S, dekodera, buforów czy priorytetów.

## Weryfikacja

382 testy hostowe i walidator GPIO PASS. Testy UI sprawdzają zachowanie
modelu przez700ms, wygaśnięcie po801ms i brak przeniesienia do nowego
streamu. Natywny build ESP-IDF6.1 nie jest dostępny w sesji z powodu
brakującego lokalnego virtualenv. Potrzebny jest odsłuch/widok OLED przy
TEST HLS64k HE-AACv2; oczekiwany brak krótkiego komunikatu bez zmiany dźwięku.
