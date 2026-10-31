# Odbiór zegara i strefy WWW

1. Zwykły flash bez erase/NVS/obrazu FS. Po STA i synchronizacji internetowej
   w prawym dolnym panelu oczekuj daty i godziny. Przed sync kreski są prawidłowe.
2. W WWW odśwież Ctrl+F5. Wybierz Konfiguracja → Data i czas. Domyślnie
   Polska/Warszawa, DST automatycznie. Wybierz UTC i zapisz: godzina OLED
   powinna zmienić się od razu, bez zatrzymania dźwięku. Przywróć wybraną strefę.
3. Restart: wybór strefy ma pozostać, clock początkowo kreski, potem bieżący
   czas po SNTP. Wi-Fi/playlista/volume mają pozostać. STOP bez stacji
   nie zatrzymuje zegara. Zakładka Status pokazuje wcześniejsze dane sieci/audio.

Pierwsza lista: UTC oraz Warszawa/Londyn/Berlin/Paryż/Helsinki/Madryt/Rzym,
bez pełnego katalogu światowych stref. Automatyczna zmiana według obecnej
reguły UE; granice DST sprawdzone testami względem zoneinfo. Nie trzeba
zmieniać czasu systemowego urządzenia do testów DST.
Brak działającego NTP oznacza kreski, nie zgadywaną godzinę. Po pierwszej
synchronizacji czas biegnie także offline; po wyłączeniu zasilania synchronizacja
potrzebna ponownie. Rzeczywisty dryf i długi brak NTP nie zmierzone.
