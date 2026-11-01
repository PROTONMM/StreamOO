# ADR0088 — transfer OLED tylko po zmianie pikseli

Data:2026-11-01. Status: implementacja; nowy tryb wymaga odbioru S3.

Użytkownik zaproponował wysyłki zależne od zawartości bufora: prezentacja
RMS i maksymalne tempo FFT to około10Hz, a cały obraz wysyłano25 razy/s.
Osobno potwierdził usunięcie wcześniejszego problemu wyświetlacza po ostatniej
kompilacji po korekcie timingów ADR0087; nie podał nowego logu/hashu.

## Decyzja

Zachować40ms rytm właściciela UI dla listy, marquee, reakcji na enkoder,
zegara i wygaszania. Po narysowaniu klatki porównać piksele z ostatnim
poprawnie przesłanym obrazem:

- identyczny obraz: zero komend i danych SPI;
- zmiana: jeden pełnej szerokości zakres od pierwszego do ostatniego
  zmienionego wiersza. Niezmienione wiersze wewnątrz mogą być przesłane,
  aby uniknąć wielu małych transakcji;
- zmiana tylko tytułu nie wysyła ponownie dolnego panelu;
- pierwszy obraz i wybudzenie: pełna klatka przed włączeniem OLED;
- błąd transferu: bez zatwierdzania kopii; unieważnienie cache i pełna
  następna próba, także po powrocie aplikacji do poprzednich pikseli.
  RAM panelu mógł zostać częściowo zmieniony przed zgłoszeniem błędu.

Cache8192B PSRAM alokowany przy open, oddzielny od wewnętrznego staging
DMA8192B. Częściowy transfer nadpisuje staging, więc nie może on służyć
jednocześnie jako kopia ostatniej pełnej klatki. Brak alokacji w present,
zmian PCM/DSP i wywołań z I2S. Porównanie memcmp wierszy, bez hashy.
Zatwierdzany tylko zakres poprawnie przesłanych pikseli.

Stare present wymusza pełny transfer; present_changed używany w zwykłym UI.
Skończona diagnostyka i test HOLD/REPEAT zachowują wymuszone transfery.
To synchronizacja wysyłek ze zmianami RAM, nie sprzętowy VSYNC; panel
dalej sam skanuje GDDRAM. Profil B3/B1 pozostaje. Przy ruchomym tekście
SPI może działać częściej niż10Hz, lecz aktualizuje inne wiersze.
Rasteryzowanie i sprawdzanie klatki nadal odbywa się w rytmie UI.

## Metryki i odbiór

Co około10s: OLED UI interval_ms, checked, sent, rows, payload,
max_present_us. Payload to bajty pikseli bez komend; checked oznacza
próby prezentacji. Max obejmuje porównanie, SPI/staging i zatwierdzenie
cache. Boot i eksperyment timingów poza licznikami; po eksperymencie
reset liczników. Pauza może wydłużyć interwał. CPU mierzy profiler RTOS.

Testy: pierwsza/identyczna klatka, krawędzie0/63, kasowanie pikseli,
dokładne adresy/payload, błędy każdej z6 operacji i naprawa po częściowym
zapisie. Symulacja25 ticków UI/10 zmian miernika daje10 transmisji;
z przewijaniem25, lecz między nowymi poziomami miernika tylko wiersz tekstu.
To test kontrolowany, nie zmierzony FPS S3. C stress2000 iteracji symuluje
GDDRAM, częściowe błędy, retry i strażniki, z ASan/UBSan.

Wyniki host/build w STATUS. Agent nie flashował. Do odbioru: brak nowych
artefaktów, płynny scroll/lista, STOP/PLAY i spadek payload przy nieruchomym
tekście. Nie zmieniać jednocześnie źródła FLAC ani profilu OLED.
