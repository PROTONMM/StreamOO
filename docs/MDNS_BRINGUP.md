# Odbiór mDNS w LAN

Wgraj firmware standardowo, bez erase/NVS. Po połączeniu z Wi-Fi/AP-off
w UART będzie `discovery: mDNS started: http://streamoo-xxxxxx.local/`.
Po rozstrzygnięciu konfliktu może pojawić się nowy `Player address`.
Otwórz wskazany pełny adres HTTP na komputerze/telefonie w tej samej sieci.
Sufiks to sześć znaków z nazwy AP, małymi literami. WWW ma pokazać odtwarzacz.
Adres IP na OLED nadal działa; wygląd WWW jest odłożony do dopracowania.

Sprawdź PLAY i czyste audio, zmianę stacji, potem ponowne połączenie Wi-Fi
oraz reboot. Po reconnect nazwa powinna ponownie działać; w AP pozostaje
konfiguracja pod192.168.4.1. Podeślij log sieci/discovery/player oraz heap
po kilku minutach grania. Nie przesyłaj haseł. Odbiór audio/multicast i
pamięci tego etapu OPEN do testu płytki.

Na Linux z Avahi opcjonalnie `avahi-resolve-host-name streamoo-xxxxxx.local`
i `avahi-browse -rt _http._tcp`. Brak obsługi mDNS/izolacja klientów lub
blokada multicast w routerze może uniemożliwić .local mimo działającego IP.
Nazwa edytowana przez WWW i jej NVS są jeszcze OPEN.
