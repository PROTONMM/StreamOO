# ADR0026 — pierwszy natywny SoftAP i prywatny build

Data:2026-10-31. Status: zaimplementowane; odbiór09a na S3 OPEN.

## Kontekst i zakres

AAC06h odebrane. Przed czytnikiem sieciowym sprawdzamy Wi-Fi SDK i współpracę
z audio. Mały krok09a to zabezpieczony AP, nie całe provisioning09. Nie dodajemy
STA/portalu/mDNS/HTTP ani nie zmieniamy produkcyjnej polityki startu radia.

## Zależności i API

Nowy komponent network używa esp_wifi, esp_netif, esp_event, esp_hw_support,
nvs_flash i FreeRTOS z przypiętego ESP-IDF6.1. Publiczne API i ograniczenia
sprawdzono w lokalnych nagłówkach: create/destroy_default_wifi_ap, init,
set_storage RAM, mode AP, set_config, start/stop/deinit, DHCP server, MAC.
Nagłówki SDK i esp_wifi/lib/LICENSE mają Apache2.0; notice zachowany obok
istniejącej licencji Apache. Bez nowego pakietu registry i kopiowania kodu.
Wyłącznie ESP32-S3/2.4GHz; profil max4 klientów, kanał1, WPA2-PSK, PMF optional.
Region/RF/zasięg/zgodność klientów wymagają testu, nie zakładamy odbioru.

## Hasło

Wspólne dla urządzeń, bez domyślnej wartości. Interaktywne narzędzie Python
z getpass zapisuje JSONv1 poza repo (0600, atomowo), nigdy w argumentach
komend. Klucz8..63 drukowalne ASCII. CMake generuje prywatny header tylko
do ignorowanego build/ lub zewnętrznego katalogu budowania. Nazwy/treść haseł
nie trafiają do logów CMake/firmware, błędy parsera są redagowane. Cache trzyma
samą opcjonalną ścieżkę. Domyślny brak pliku tworzy wariant bez sekretu i nie
uruchamia Wi-Fi. Brak/błąd jawnego pliku blokuje konfigurację. Nie ma otwartego
fallback AP ani przykładowego klucza używanego w urządzeniu użytkownika.
Firmware zawiera klucz, nie deklarujemy jego nieodczytywalności.

## Runtime

Oddzielny worker network: priorytet3/stos6144B, AP SSID StreamOO-XXXXXX
z MAC SoftAP. Jawny adres192.168.4.1/24 i DHCP, max4 klientów. Operacje SDK
w zadaniu, poza UI/enkoderem i ścieżką próbek. Sterownik konfigurowany w RAM,
bez zapisu AP credentials do NVS. nvs_flash_init na potrzeby SDK bez erase,
także przy błędzie wersji/pełnej pamięci. Co10s liczba klientów i stos workera.

Po błędzie log kodu bez klucza, cleanup drivera/netif i zakończenie taska;
brak automatycznego restartu/usuwania NVS. Globalna inicjalizacja netif/event
loop pozostaje jednorazowa; pełny stop/restart managera jest poza tym etapem.
Audio nadal czyta lokalny klip, HOME ma syntetyczny network snapshot.
Wyłączenie AP po pomyślnym STA nastąpi wraz z implementacją STA; teraz go nie ma.

## Partycja

Rzeczywisty obraz AAC+Wi-Fi miał0x10dab0B i przekroczył1MiB o0xdab0B.
Przechodzimy na gotową PARTITION_TABLE_SINGLE_APP_LARGE SDK (1500KiB).
Nie kopiujemy CSV. NVS0x9000/0x6000, phy0xf000/0x1000 i factory0x10000
bez zmiany położenia; tylko factory rośnie z1MiB do1500KiB. Flash zwykłego
obrazu z tablicą, bez erase; NVS preferencji zachowane. Docelowe OTA/LittleFS
nadal wymagają osobnego obliczenia podziału16MiB, nie udajemy ich wdrożenia.

## Weryfikacja i odbiór

Host testuje zgodność polityki C/Python dla klucza, format SSID i granice,
prywatny zapis/permisje, header/no-secret, walidację/redakcję błędów i zakaz
pliku z sekretami w repo. Testowe klucze generowane losowo w /tmp, bez
zapisu wartości w źródłach/test logach. Native build uruchamia ścieżkę SDK
z takim kluczem; końcowy lokalny build nie zawiera klucza testowego.
Wyniki build/test w STATUS. Nie flashujemy za użytkownika.

Odbiór: AP ready, WPA2 association, DHCP192.168.4.x, clients>=1/0 po
połączeniu/rozłączeniu, odmowa złego klucza oraz czyste AAC i stabilne
PCM/heap przy aktywnym RF. Brak danych hardware oznacza OPEN. Po odbiorze
portal/STA rollback, potem HTTP/HTTPS. Procedura NETWORK_BRINGUP.md.
