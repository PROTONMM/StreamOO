# Odbiór07b — rzeczywiste RMS/peak na OLED

1. Wgraj zwykłym `idf.py -p PORT flash`, zachowaj NVS/LittleFS, bez erase
   i obrazu FS. Wybierz znaną działającą stację.
2. Wciśnij enkoder i obracaj, wybierz bargraph L/R albo dwa mierniki
   analogowe. Powinny reagować na audio. FFT nadal pokazuje „Brak danych”.
3. Zmień volume na0,50,100. Wskaźniki mają nadal reagować przy mute, bo
   pomiary są przed gain. Dla zmiennej audycji wartości naturalnie się
   zmieniają — porównanie liczb wymaga tego samego fragmentu/sygnału.
   Test stałych próbek przez rzeczywisty pcm_fill/tap/gain już PASS na hoście.
4. STOP ma usunąć poziomy (placeholder), PLAY przywraca je po nowym oknie
   i wyjściu z buffering. Zmień stację, także szybko dwa razy: poziomy starej
   stacji nie mogą pozostać. Odtwarzanie podczas przeglądania listy zostaje.
5. Sprawdź płynność i brak trzasków. Zbierz około15–30s logu player/platform
   przy odtwarzaniu i po kilku zmianach, bez prywatnych URL/credentials.
   Istotne: max_fill/invalid/low/hard, heap internal free/min/largest,
   `meters valid/windows/gaps/drop/max_work/stack_hwm/rms/peak`.

Max_work to czas ścienny iteracji konsumenta, również możliwe wywłaszczenia,
nie pomiar procentowego CPU. Spodziewane normalnie gaps/drop0; sporadyczne
odrzucenie pomiarów nie powinno powodować przerwy audio. Test sztucznego
pełnego bufora na hoście potwierdza drop-newest i reset okna po luce; nie
zastępuje pomiaru wydajności na urządzeniu. Długotrwałe przeciążenie powoduje
„Brak danych”, wynik starszy niż200ms jest unieważniany.

Błąd `Audio meters disabled` oznacza brak pamięci/zadania; audio ma grać
dalej. Sprawdź wtedy heap. Około10KiB kontekstu i4096B stosu plus TCB
tworzone na sesję; po STOP kontekst zwalniany po ACK, stos przez idle task.

07c: okna pomiarowe40ms, prezentacja10Hz, RMS attack30/release150ms,
peak hold150/release300ms. Sprawdź spokojniejszy ruch i brak migania przy
krótkich lukach tap. STOP/zmiana stacji nadal czyści od razu, a brak nowych
kompletnych pomiarów przez200ms daje placeholder. Dalsze strojenie skali
dB/VU oraz FFT/Hann/ESP-DSP pozostają kolejne.
