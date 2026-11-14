# Odbiór WWW AP/LAN i playlisty

Bieżąca instrukcja użytkowa: [USER_GUIDE.md](USER_GUIDE.md).
Aktualizacja 2026-11-14: panel ma zakładkę Ustawienia; w LAN import jest
w Playlista, a w AP na stronie konfiguracji. Import dopisuje stacje
(ADR0070), zamiast zastępować katalog.

1. Zwykły flash bez erase/NVS/obrazu FS. WWW Ctrl+F5. W AP oczekuj tylko
   Wi-Fi/importu/strefy czasu, bez playera i edytora. Po STA/AP-off domyślnie
   Odtwarzacz, z listą stacji i PLAY/STOP/volume. Konfiguracja nadal dostępna
   w LAN osobno dla sieci/zegara.
2. Kliknij dwie różne stacje na WWW: OLED i dźwięk mają przejść na wybraną.
   Sprawdź też encoder → zaznaczenie w WWW w kolejnym statusie. Brak stacji
   oznacza wskazówkę do importu/dodania w zakładce Playlista.
3. Wyeksportuj kopię obecnej listy jako backup. Eksport obejmuje zapisaną
   listę. Dodaj testową stację nazwą/URL w edytorze, zapisz; oczekuj także
   aktualizacji listy OLED. Popraw nazwę/URL i usuń testowy wiersz, zapisz.
   ovol istniejących pozycji zachowane, nowe0.
4. Importuj poprawną kopię: preview i osobne zatwierdzenie. Niezapisana
   edycja ma wymagać potwierdzenia przed jej odrzuceniem. Błędny CSV nie
   zmienia zapisanej listy.
5. Dwie przeglądarki: w obu odczytaj, zapisz zmianę w pierwszej, potem
   spróbuj zapisać stare dane z drugiej. Oczekuj konfliktu bez nadpisania
   i zachowanego draftu. Odczytaj aktualną listę przed następną zmianą.
6. Czyszczenie całej listy jest opcjonalnym testem destrukcyjnym dla katalogu:
   najpierw backup. Po potwierdzeniu lista pusta, aktywna stacja zatrzymana.
   Można odtworzyć eksport przez import. Pusty eksport to0B; do celowego
   czyszczenia użyj przycisku, nie importu pustego pliku.

Zmiana nazwy/adresu aktualnej stacji zachowuje ID i odtwarzanie; nowy adres
obowiązuje przy następnym połączeniu (ADR0059). Limit 2048 stacji,
256 KiB CSV; odtwarzacz ma pełną przewijaną listę bez stron (ADR0107),
edytor nadal pokazuje po 25 stacji.
Agent nie uruchamiał tych operacji na urządzeniu ani prywatnej playliście.
NVS strefy po fizycznym power-cycle użytkownik sprawdza osobno.
