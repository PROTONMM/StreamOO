# Odbiór 09c — konfiguracja WWW, sieci i playlista

Aktualny firmware zastępuje historyczny portal odczytu opisany niżej.
Decyzje i limity: ADR0031.

1. Wykonaj zwykłe `idf.py -p PORT flash` z aktualnym prywatnym kluczem AP.
   Ten etap zmienia tablicę partycji: potrzebny jest pełny flash, obejmujący
   tablicę, zamiast samego app-flash. Nie kasuj flash/NVS i nie wgrywaj obrazu FS.
2. Połącz się z AP, otwórz http://192.168.4.1/ i odśwież Ctrl+F5.
   Widać sekcje „Zapisane sieci” i „Wgraj playlist.csv”. Wpisz hasło AP
   z OLED w „Dostęp do konfiguracji”, wybierz odczyt ustawień. Błędne hasło
   ma blokować operacje; hasło nigdy nie jest zwracane przez API.
3. Najpierw przetestuj upload prywatnej playlisty: wybierz plik, sprawdź,
   oczekuj 267 stacji, 11 oczyszczonych adresów, 3 zachowanych powtórzeń.
   Dopiero „Zatwierdź i zapisz” zastępuje katalog. Zrestartuj i odczytaj
   ponownie: 267 stacji ma pozostać. Błędny plik nie zmienia katalogu.
   Oryginał lokalny nie jest edytowany ani dołączany do firmware.
4. Dodaj sieć domową i zapisz. Przyjęcie listy jest asynchroniczne; czekaj
   na próbę STA (do 15 s na profil). Po IP lista zostaje zapisana, AP
   wyłącza się po 5 s. Połącz komputer/telefon z domową siecią i otwórz
   adres widoczny na OLED. Przy izolacji klientów router może blokować dostęp.
5. Sprawdź dwie sieci: nieosiągalna/błędne hasło pierwszej, poprawna druga;
   zmiana kolejności przyciskiem ↑/↓. Sprawdź, że błędna cała nowa lista
   zachowuje poprzednią. Po restarcie urządzenie próbuje listę z NVS.
   Restart udostępnia ponownie AP na czas prób; po udanym IP znowu go wyłącza.
6. Zmieniając hasło istniejącego SSID, wpisz nowe; puste pole zachowuje
   stare. Sieć otwarta wymaga jawnego zaznaczenia checkboxa. Usunięcie
   wszystkich wierszy i potwierdzenie zapisuje pustą listę i utrzymuje AP.
7. Oczekuj ciszy, bez fixture_reader/codec/audio_control; obecny boot setupu
   nie uruchamia odtwarzacza także po STA. Import przechowuje katalog,
   nie zmienia jeszcze syntetycznej listy OLED i nie uruchamia streamów.

Do oceny prześlij wyniki formularzy/reboot/AP-off i logi network/portal/heap,
bez haseł, Authorization, nazw/URL prywatnych stacji. Zmierz wolną pamięć
i stack_hwm po największym uploadzie. Fizyczne power-cut/odporność flash OPEN.
Lokalny test formularzy bez PCB: `python3 tools/check_portal_browser.py`
(wymaga Chromium /usr/bin/google-chrome i lokalnych socketów, dane syntetyczne).

---

# Historyczne procedury 09a/09b

Poniższe opisy zachowano jako historię poprzednich wersji, nie aktualne
instrukcje. W 09c klucz AP jest wysyłany do urządzenia w nagłówku Basic
przy operacjach setupu; nadal nie pojawia się w odpowiedziach HTTP ani UART.

# Wi-Fi09a — zabezpieczony SoftAP

To test natywnego Wi-Fi równolegle z odebranym lokalnym audio AAC. Nie ma
jeszcze portalu WWW, STA, dostępu do Internetu, mDNS ani readera HTTP.
Nie zmienia istniejących preferencji volume/widżetu i nie kasuje NVS.

## Prywatne hasło i budowanie

W terminalu na swoim komputerze, w repozytorium:

```sh
python3 tools/configure_network.py
source /home/mariuszk/.espressif/tools/activate_idf_v6.1.sh
export PATH=/usr/bin:$PATH
idf.py reconfigure build
```

Podaj wspólne hasło AP i potwierdzenie w ukrytym wejściu terminala. Obsługiwane
8..63 drukowalne znaki ASCII. Nie przesyłaj hasła w czacie ani w argumentach
komend. Skrypt nie generuje hasła per urządzenie: tej samej wartości używamy
przy budowaniu firmware dla wszystkich urządzeń.

Prywatny JSONv1 zapisuje się poza repo: domyślnie
`~/.config/streamoo/network.json` (lub `$XDG_CONFIG_HOME/streamoo/network.json`),
z uprawnieniami0600 i atomową podmianą. Nie edytuj sdkconfig ani defaults,
żeby wpisać hasło. Można podać `--file /zewnetrzna/sciezka/network.json`;
wtedy skrypt wypisze odpowiednie polecenie CMake z samą ścieżką. Plik wewnątrz
repozytorium, również w ignorowanym secrets/, jest odrzucany.

Nagłówek z kluczem jest prywatnym artefaktem w ignorowanym build/, nie plikiem
źródłowym; klucz znajduje się także w zbudowanym firmware (nie jest to ochrona
przed jego odczytem z urządzenia). Manifest/lock nie zawierają haseł. Edycja
istniejącego pliku jest zależnością CMake; po jego pierwszym utworzeniu użyj
jawnie `idf.py reconfigure build`.

Bez domyślnego pliku build jest możliwy, ale `AP test inactive:
ESP_ERR_NOT_FOUND` oznacza brak Wi-Fi. Nie ma otwartego AP ani hasła
zastępczego. Jawnie wskazany błędny/brakujący plik przerywa konfigurację
z komunikatem bez wartości. Wyłączenie STREAMOO_NETWORK_AP_TEST również
wyłącza test. Zmiany ustawień AP nie są zapisywane przez sterownik do NVS
(WIFI_STORAGE_RAM); inicjalizacja RF nadal korzysta ze standardowego NVS SDK.

## Flash i odbiór

Obraz z Wi-Fi przekroczył1MiB. Profil używa teraz gotowej partycji SDK
`PARTITION_TABLE_SINGLE_APP_LARGE` (1500KiB). NVS nadal0x9000/0x6000,
phy_init0xf000/0x1000, factory od0x10000. Zmienia się tylko rozmiar factory.
Jeśli używasz własnego sdkconfig sprzed tego etapu, wybierz w menuconfig
Partition Table → Single factory app (large); nowe defaults nie nadpisują
istniejących wyborów. W bieżącym workspace profil już przestawiony.
Przy pierwszym wgraniu tego etapu użyj zwykłego `idf.py -p PORT flash`
z tablicą partycji, nie samego app-flash. Nie wykonuj erase-flash.

1. Po wgraniu oczekuj `network: AP ready: SSID=StreamOO-XXXXXX WPA2;
   IPv4=192.168.4.1; TEST no portal/STA`.
2. Połącz telefon/komputer z tym SSID i podanym lokalnie hasłem. Sufiks pochodzi
   z MAC SoftAP. Sieć nie daje Internetu; pozostanie połączonym jest oczekiwane.
   Sprawdź przydzielony DHCP adres192.168.4.x i bramę192.168.4.1. Nie oczekuj
   strony WWW pod tym adresem. HOME nadal pokazuje testowe dane sieciowe.
3. Log co10s: `network: AP clients=N stack_hwm=...`. Potwierdź N>=1 po
   połączeniu, potem0 po rozłączeniu. Złe hasło nie powinno pozwalać dołączyć.
4. Włącz lokalne AAC PLAY, posłuchaj kilku cykli i sprawdź VOL/STOP/PLAY.
   Podeślij AP ready/clients, PCM/buffers/reader/platform heap oraz wynik
   odsłuchu. Hasła nie powinno być w logu i nie należy go dopisywać.

Oczekujemy stabilnej pamięci, low0/hard0/invalid0 i czystego dźwięku przy
połączonym kliencie AP. To nowy test współpracy RF/sterownika z audio;
odebrane06h go nie zastępuje. Wydajność, zgodność klienta/PMF, zasięg,
niska pamięć i długotrwały test pozostają OPEN do pomiarów.

Następny krok po odbiorze: portal konfiguracji i test STA z rollback, dopiero
potem reader HTTP/HTTPS. Hasło wspólnego AP nie rozstrzyga uwierzytelniania
administracyjnego przyszłego WWW (D18 nadal osobno OPEN).

## Pojedyncze nieczytelne znaki konsoli

W logu e369d49 odnotowano pojedynczy uszkodzony fragment fixture_reader
przy potwierdzonym dotykaniu przewodów UART. Format tego wpisu to ASCII:
`fixture_reader: generation=<liczba> input=37039 stack_hwm=<liczba>`.
Po nim cykle/PCM pracowały dalej bez błędów. Zabezpiecz połączenie TX/GND
na próbę bez dotykania; monitor UART115200. Przy powtórzeniu bez ruchu
potrzebne oryginalne bajty z odbiornika, ponieważ kopiowanie tekstu z Unicode
replacement traci dane. Jeśli płytka udostępnia natywne USB Serial/JTAG,
secondary output jest włączony: porównanie obu wyjść pomoże oddzielić tor
UART/adapter od wspólnej ścieżki logowania. Przyczyny nie uznajemy za dowiedzioną
ani nie zmieniamy firmware wyłącznie na podstawie takiego fragmentu.

Ponowna próba użytkownika2026-10-31 potwierdziła przyczynę: poruszanie
przewodów UART przy dotykaniu enkodera. Po sprawdzeniu „Jest OK”. Obserwacja
zamknięta, bez zmiany firmware; opis porównania konsol pozostaje tylko
procedurą na wypadek nowego, niezależnego objawu.

## Portal09b i ekran AP (bieżący obraz)

Używa tego samego, stałego klucza prywatnego buildu. Wystarczy zwykły build
w istniejącym środowisku i flash; nie trzeba ponownie podawać hasła. Nowa
partycja1500KiB pozostaje bez zmian. Bez prywatnego pliku nadal nie ma AP.

1. OLED ma pokazać SSID, hasło AP i `http://192.168.4.1`. Hasło dłuższe niż
   42 znaki kontynuuje się w następnym wierszu bez separatora. AP aktywne→
   ekran konfiguracji zamiast HOME/listy. Obrót reguluje VOL, trybA długi klik
   PLAY/STOP (trybB krótki); gest listy i pressed-rotate pomijane podczas AP.
2. Połącz klienta z AP i otwórz **http://192.168.4.1/**. Strona działa offline,
   bez CDN i bez wymaganego Internetu. To panel odczytu, nie ma jeszcze
   formularza domowej sieci. Portal nie przechwytuje DNS/captive detection.
3. Panel powinien pokazać nazwę AP i rzeczywiste VOL/PLAY/STOP zmieniane
   enkoderem; poll co2s. Liczba klientów pochodzi z pomiaru sieci co10s.
   JSON pod `/api/v1/status` ma schema_version1 i nie może zawierać hasła.
4. Sprawdź stronę na telefonie i wykonaj kilkanaście odświeżeń podczas AAC.
   Metody zmieniające stan nie istnieją; POST/api/v1/status ma być odrzucony.
   Nie podawaj hasła w URL ani w logu do przesłania.
5. Podeślij portal ready, network/PCM/buffers/heap oraz potwierdzenie OLED,
   strony, zgodności volume i czystego dźwięku. Oczekujemy low0/hard0/invalid0
   i stabilnej pamięci przy otwartej stronie.

Hasło AP nie trafia do HTTP ani UART; jego wyświetlanie na OLED jest
zamierzone i autoryzowane. Wyłączenie AP po połączeniu STA/IP jest wymaganiem
następnego zakresu, obecny firmware nie konfiguruje jeszcze STA. Nie wyłączamy
AP po samym połączeniu telefonu/komputera z punktem dostępu urządzenia.

## Aktualizacja: AP bez odtwarzania

Po zaliczeniu odsłuchu testowego użytkownik wyłączył odtwarzanie w trybie
konfiguracji AP. Zwykły build/flash z dotychczasowym kluczem. Oczekuj
`Audio disabled: AP configuration mode`, braku logów fixture_reader/codec/
audio_control i braku tonu. Enkoder nie powinien uruchomić dźwięku ani
zmienić preferencji na ekranie AP. OLED z SSID/hasłem/URL i portal działają;
strona mówi „Audio wyłączone w trybie AP”, głośność jest niedostępna.
To nowa procedura zamiast wcześniejszego polecenia PLAY podczas AP.
Do testów samego audio wyłącz NETWORK_AP_TEST; MicroSD nadal przyszłościowe.
