# ADR0084 — bargraph z szachownicowym wypełnieniem

Data: 2026-11-01. Status: build/host PASS, wygląd i pulsowanie OLED OPEN.

Użytkownik potwierdził poprawny wygląd logo oraz brak pulsowania widżetu
ze wskazówkami. Zaproponował szachownicowe wypełnienie bargraphu i ciągłą
ramkę. Nie ustalono jeszcze przyczyny pulsowania wypełnionych słupków.

Decyzja: każdy aktywny słupek RMS ma ciągły obrys GS10 oraz szachownicę
1×1 px GS10/czerń we wnętrzu. Obrys rośnie razem z szerokością RMS.
Pionowy peak pozostaje ciągły GS15, także poza bieżącym RMS. Przy RMS=0
brak obrysu i wypełnienia, ale peak może pozostać widoczny. Parzystość
szachownicy jest związana z pozycją kanału, nie poziomem lub czasem.

Dla pełnego słupka 132×14 świeci 1068 zamiast 1848 pikseli, czyli około
42% mniej. To liczba pikseli, nie pomiar redukcji prądu ani naprawy migotania.
Kontrast, profil SSD1322, FPS, pomiary RMS/peak, audio i spectrum bez zmian.
Wyłącznie renderer, bez nowych zależności i alokacji w pętli rysowania.

Weryfikacja: 423 testy hostowe PASS; istniejący test bargraphu rozszerzony
o ciągłość obrysu/peak, teksturę, redukcję powierzchni i niezależność kanałów.
ASan/UBSan istniejącego stress-testu różnych bounds/poziomów PASS;
LeakSanitizer wyłączony ze względu na ptrace środowiska (renderer nie alokuje).
ESP-IDF6.1 build dr PASS. Podgląd z rzeczywistego renderera w
build/bargraph-checker.png (powiększenie4×, umowna żółta paleta).
Agent nie flashował. Odbiór na panelu i wpływ na pulsowanie: OPEN.
