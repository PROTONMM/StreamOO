# Odbiór WWW AP/LAN i playlisty

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

Zmiana nazwy/adresu aktualnej stacji może ją zatrzymać; niezmienione ID
podczas edycji innych pozycji ma zachować odtwarzanie. Limit2048 stacji,
256KiB CSV; player50/stronę, edytor25.
Agent nie uruchamiał tych operacji na urządzeniu ani prywatnej playliście.
NVS strefy po fizycznym power-cycle użytkownik sprawdza osobno.
