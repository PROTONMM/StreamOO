# ADR0036 — STA przed AP i wiersz IP/głośności

Data: 2026-10-31. Status: przyjęte na polecenie użytkownika; odbiór sprzętowy OPEN.

## Decyzja

Zastępujemy uruchamianie AP przy każdym boot: najpierw zapisane profile STA
w kolejności, po pierwszym IPv4 normalny HOME bez AP. Brak profili albo
niepowodzenie całej listy uruchamia bezpieczne AP i ekran konfiguracji.
Przy zaniku sieci również sprawdzamy całą listę przed AP. Błędna zmiana
WWW lub błąd zapisu NVS przywraca poprzednie profile przed fallback AP.
AP wcześniej uruchomione wyłącza się5s po IP. Hasło nadal prywatnie wbudowane.

Pozostaje limit15s/profil; FAST_SCAN, bez niepotrzebnego rozłączenia i czekania
przed pierwszą próbą. Konfiguracja AP przygotowana przy zatrzymanym Wi-Fi,
tryb STA ustawiony przed esp_wifi_start. Nie wykonujemy dodatkowego pełnego
skanu sieci. Rzeczywisty czas startu/RF wymaga pomiaru na płytce.

HOME y24..31: IP x0..127 jasność6, VOL x128..171 jasność15, pozioma belka
x176/y25/80×6 jasność15. Wypełnienie wnętrza78×4 odpowiada volume0..100.
To ustawienie głośności; analiza PCM/DSP pozostaje osobnym etapem. Własny
widget i kompozytor canvas, bez alokacji, I/O i zmian callbacku I2S.

## Weryfikacja

266 testów hostowych PASS: rzeczywisty owner sieci z atrapami SDK/RTOS/NVS
obejmuje12 scenariuszy startu, kolejności, fallback, rollback i AP-off.
Testy rastra sprawdzają wszystkie101 poziomów, clipping, kasowanie i jasności.
Native ESP-IDF6.1 build PASS, obraz0x15b530, 7% wolnej partycji app.
Brak nowych zależności/kodu stron trzecich. Bez flash/erase/eFuse.
Testy atrap nie potwierdzają czasu RF/DHCP ani czytelności fizycznego OLED.
