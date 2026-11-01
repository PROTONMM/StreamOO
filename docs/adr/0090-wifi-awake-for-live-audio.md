# ADR0090 — wyłączenie modem sleep podczas pracy radia

Data: 2026-11-01. Status: eksperyment do odbioru na ESP32-S3.

## Wynik pomiarów ADR0089

Załącznik 3d8f1f16-9f35-41b8-bdfa-62ffa7771f1f, SHA256:
`57765197321faa63926e4b5d4ede8cc1accdfdd3c06bbb41e82cebf94c4f79ae`.
Firmware bf40138, ELF dbb91c37, IDF6.1, 240MHz, dr_flac-0.13.4,
TEST FLAC native lossless. Zapis do327,088s. Brak nowej deklaracji odsłuchu;
wcześniej użytkownik nie miał możliwości słuchania, więc odsłuch OPEN.

- 97 raportów Underrun, starts98, hard0, max_fill90us. 45 ostrzeżeń
  z compressed=1; 21 z RX age>100ms. Mediana znanych RX age44,5ms,
  wiele przypadków z nowym body i nadal małym PCM. Nie każdy niedobór
  wymagał długiej przerwy po stronie HTTP.
- Największy zmierzony ścienny HTTP read673902us, pakiet logów82763us.
  Te czasy obejmują przełączenia zadań; nie są pomiarem samego serwera
  ani samego UART. Logowanie zabiera istotną część zapasu PCM160ms,
  ale nie tłumaczy wszystkich zdarzeń.
- 63 interwały body HTTP:529..1491kbps. Nie jest to bitrate dekodera ani
  bezwarunkowy limit przepustowości łącza. Przy pełnym buforze czytnik
  czekał na zapis do ring nawet2169528us. Przykład291,578s: RX age1850ms,
  lecz brak nowego low; później PCM32/ring prawie pełny. To ważne
  rozróżnienie backpressure od niedostępnego serwera.
- 31 interwałów CPU: core0 średnio43,19%, max48,2%; core1 średnio43,31%,
  max49,3%. Końcowe FFT avg2846us/max7710us, gaps1/drop2. Zapas średniego
  CPU, bez dowodu stale przeciążonego dekodera/FFT.
- Dwa Recover:64,288s skipped23735/validated18243/CRCf282 i185,478s
  skipped33043/validated21256/CRC9fc4. Brak Reject/reconnect po starcie gen3.
  Odzyskiwanie działa, lecz źródło błędnych bajtów z ADR0086 nadal OPEN.
  Początkowe dwie porażki TLS są osobnym problemem przed odtwarzaniem.
- 32 raporty OLED:7976 checked/3523 sent, payload7893120B wobec
  65339392B przy pełnym wysłaniu każdej sprawdzanej klatki. Około87,92%
  mniej payload według liczników sterownika, bez liczenia komend SPI.
  ADR0088 działa na S3; to nie ocena wizualna ani pomiar energii panelu.

W 3,098s sterownik raportuje `pm start, type: 1`. Kod nie wywoływał
esp_wifi_set_ps. SDK6.1 w esp_wifi.h potwierdza domyślny WIFI_PS_MIN_MODEM;
wifi_ps_type_t definiuje NONE=0, MIN_MODEM=1. Mamy więc konkretny punkt
do zmiany, a nie tylko wnioskowanie z czasu między odczytami.

## API i źródło

[Oficjalna dokumentacja ESP-IDF6.1 dla S3 — Station Sleep](https://docs.espressif.com/projects/esp-idf/en/release-v6.1/esp32s3/api-guides/wifi-driver/wifi-performance-and-power-save.html#station-sleep)
opisuje domyślny MIN_MODEM, odbiór zależny od DTIM oraz WIFI_PS_NONE jako
wyłączenie modem sleep i ograniczenie opóźnienia odbioru kosztem energii.
API esp_wifi_set_ps/get_ps sprawdzone również w lokalnym SDK6.1,
components/esp_wifi/include/esp_wifi.h. Istniejąca zależność esp_wifi,
bez nowej biblioteki/importu kodu stron trzecich.

## Decyzja i granice

Po esp_wifi_start, przed próbami profili STA, ustawić WIFI_PS_NONE.
Jednorazowy getter potwierdza faktyczny tryb. INFO `Wi-Fi power save=NONE;
low-latency RX` tylko po udanym setter/getter i zgodnym odczycie. Na błędzie
lub niezgodności WARN z kodami/trybem; uruchomienie sieci i portalu trwa
dalej. Ustawienie nie trafia do flash, jest ponawiane po każdym restarcie
właściciela sieci. Dotyczy połączonej stacji Wi-Fi, także podczas STOP;
nie przywracamy sleep między stacjami ani podczas chwilowego rebuffer.

To celowy wybór priorytetu opóźnień nad oszczędzaniem energii. Nie zmieniamy
rozmiaru ring, preroll/refill, okna TCP, liczby buforów Wi-Fi, mocy TX,
dekodera, meteringu, logów playera, I2S ani ustawień OLED. Dzięki temu
następna próba izoluje jedną politykę transportu. Wyłączenie modem sleep
nie dowodzi naprawy CRC ani nie gwarantuje dostarczenia body przez Internet.

Aktualna konfiguracja ma także TCP WND5760B/MSS1440B. Małe okno może
ograniczać przepustowość przy opóźnieniach WAN; bez pomiaru RTT nie
uznajemy go za przyczynę. Ewentualna próba okna TCP powinna być osobnym
krokiem, z obserwacją wolnego internal RAM. Nie utożsamiać PSRAM ring256KiB
z oknem odbiorczym TCP ani z dostępną pamięcią drivera Wi-Fi.

## Weryfikacja

Testy rzeczywistego ownera z fake SDK sprawdzają kolejność start→set/get→
connect, jednorazowość także podczas zmian profili/AP i prawdziwy readback.
Trzy nowe przypadki: błąd set, błąd get, getter zwraca nadal MIN_MODEM;
każdy zachowuje działające STA i nie loguje fałszywego sukcesu.
Istniejące scenariusze AP/rollback/provisioning nadal obowiązują.
Pełne wyniki host/build w STATUS. Agent nie flashuje.

Odbiór: ten sam dr/FLAC i sieć, 3–5min od resetu, potwierdzenie nowej wersji
i `power save=NONE`, Delivery/Underrun/Recover/CPU/OLED UI. Porównać tempo
narastania low i wait_empty, opóźnienia read i napełnienie przy różnych
utworach. Odsłuch gdy dostępny. Live zmienia materiał, więc sam mniejszy
licznik nie stanowi kontrolowanego benchmarku. Gdy niedobory pozostają:
osobno zbadać dopływ LAN/WAN, TCP i blokowanie logami; capture CRC z S3
nadal potrzebny do rozstrzygnięcia pochodzenia błędnych bajtów.
