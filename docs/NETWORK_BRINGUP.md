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
