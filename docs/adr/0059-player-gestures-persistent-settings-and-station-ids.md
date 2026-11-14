# ADR0059 — pauza OLED, obsługa listy i trwała tożsamość radia

Data:2026-10-31. Status: implementacja gotowa, odbiór PCB OPEN.

Użytkownik zlecił długi STOP/wygaszenie tylko OLED, krótki PLAY przy STOP,
listę przy PLAY i konfigurowalną bezczynność. Zlecił też punkty2/3/4:
ostatnią stację/tryb startu, trwałe ID niezależne od edycji i nazwę mDNS.

Przyjmujemy5s/auto-wybór, STOP po boocie i nazwę MAC jako wartości
początkowe. LAN WWW zmienia czas1..120s, auto-wybór/powrót, autoplay,
nazwę; NVS jest osobnym komponentem. Ostatni ID zapisuje worker po5s,
bez flash w ownerze audio/I2S. Autoplay zachowuje bramkę STA/AP-off.
STOP zamyka źródło, nie jest timeshiftem. Ekran wygaszany sprzętową
komendą display-off; CPU/Wi-Fi/WWW nadal działają. Krótki PLAY od razu
uruchamia pobieranie i ekran. Samo odtwarzanie wymaga buforowania źródła.
Tryb B pozostaje alternatywnym historycznym układem Kconfig.

UI i player tłumią ponowny wybór grającego ID. To dotyczy też retry/
buforowania i WWW: nie resetuje generation, źródła, buforów ani dekodera.
Edytor utrzymuje ID w żądaniu z CAS. Pojedynczy plik LittleFS v2 zawiera
ID i dane, z dotychczasowym temp/fsync/rename; stary format odczytywany,
bez migracji kasującej. Nowy URL aktywnej pozycji dopiero przy następnym
połączeniu, nazwa aktualizowana ze snapshotu. Publiczny CSV bez zmian.

Nazwa DNS jest sprawdzana przed NVS; sieciowy owner restartuje wyłącznie
mDNS. Brak nowych zależności/kodu zewnętrznego, bez zmian GPIO/partycji.
Host: rzeczywiste controls/store/settings/player/discovery/REST z atrapami
SDK i Chromium390px PASS. Build ESP-IDF6.1 PASS. Testy obejmują no-op tej
samej stacji, debounce, STOP/autoplay/brak ID, rename/URL/odczyt po restarcie,
konflikty CAS, błąd zapisu i zmiany mDNS. Hardware i zaniki zasilania OPEN;
nie wykonano flash/erase/eFuse. Dokładny protokół PLAYER_SETTINGS_BRINGUP.
