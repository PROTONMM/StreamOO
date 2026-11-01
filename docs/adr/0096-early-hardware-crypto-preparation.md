# ADR0096 — sprzętowe AES/SHA z inicjalizacją przed I2S

Data: 2026-11-01. Status: eksperyment, odbiór S3 OPEN.

## Powód

Użytkownik uzupełnił ocenę: poprzedni wariant po połączeniu grał dobrze,
programowe AES/SHA łączy szybko, lecz mocno przerywa. To ważna obserwacja
przemawiająca za przywróceniem akceleracji. Log b129d3af potwierdza niedobory
i CPU z zapasem (ADR0095); sam średni CPU nie wyklucza wpływu kosztu TLS
na harmonogram odbioru. Nie uznajemy jeszcze kosztu software crypto za
udowodnioną przyczynę wszystkich niedoborów i CRC.

W [zgłoszeniu ESP-IDF #18640](https://github.com/espressif/esp-idf/issues/18640)
autor opisuje zajęcie GDMA dla kryptografii przed pierwszym I2S jako
obejście problemu AES po zamknięciu I2S. To relacja autora dla innego
urządzenia/SDK, nie zaakceptowany fix SDK6.1 ani potwierdzenie przyczyny
StreamOO. Opis współbieżnego SHA dotyczy tam PDM RX; StreamOO ma standard
TX, bez mikrofonu. Nie przenosimy tej diagnozy automatycznie.

## Sprawdzenie lokalnego SDK i licencji

ESP-IDF6.1, istniejące Mbed TLS4.1.0/TF-PSA-Crypto i porty Espressif,
Apache-2.0 zgodnie z THIRD_PARTY_NOTICES. Nie kopiowano kodu zgłoszenia
ani nowego dekodera/kryptografii. Własny probe używa publicznych API
`aes/esp_aes.h` oraz `psa/crypto.h`, jawna prywatna zależność platform→mbedtls.

Lokalne źródła: esp_aes_crypt_cbc dla wejścia1024B wywołuje DMA także
przy włączonej optymalizacji małych danych (progi128/512B). Port AES GDMA
i SHA używają wspólnego esp_crypto_shared_gdma. Kanały TX/RX są alokowane
leniwe przy pierwszej operacji, pozostają zajęte po esp_aes_free; zwolnienie
wymaga osobnej funkcji SDK, której StreamOO nie wywołuje. Inicjalizacja
przed pierwszym i2s_new_channel unika oddania kryptografii kanału używanego
wcześniej przez I2S. To wynika ze ścieżek kodu, skuteczność wobec objawu
TLS wymaga próby sprzętowej.

Nie resetujemy globalnie GDMA, nie dotykamy rejestrów/własności kanałów,
nie modyfikujemy SDK. Zajęcie pary wcześniej może zmienić numery kanałów;
kryptografia i tak potrzebowałaby jej podczas TLS. Nie twierdzimy, że
rezerwacja usuwa wszystkie możliwe problemy współbieżnego DMA.

SDK sam wywołuje psa_crypto_init przy starcie systemu (SECONDARY104).
Ta funkcja jest idempotentna; ponowne wywołanie nie inicjuje nowego RNG.
Samo PSA init nie zastępuje probe: dopiero rzeczywiste operacje zajmują
leniwe kanały GDMA. Nie zmieniamy źródła entropii ani kolejności seedowania.

## Implementacja i kontrakt

Eksperymentalny Kconfig STREAMOO_CRYPTO_EARLY_INIT domyślnie n, zależny
od S3 oraz hardware AES/SHA. app_main wywołuje przygotowanie po walidacji
platformy, przed ustawieniami, playerem, siecią i pozostałymi peryferiami.
Bez flagi funkcja jest no-op. Brak jakiejkolwiek zmiany w fill/ISR I2S.

Probe: PSA init, SHA-2561024 zer, sprzętowe AES-128-CBC1024 zer z zerowym
kluczem i IV, SHA-256 całego szyfrogramu. Oba hashe porównane z niezależnie
obliczonymi referencjami Python hashlib/OpenSSL. To publiczne dane testowe,
bez sekretów urządzenia. Hash pełnego wyjścia wykrywa również błąd w środku
bufora. Dwa bufory po1024B z internal DMA heap zwalniane przed startem
audio, kontekst AES zwalniany także po błędzie. Nie zwiększamy stale PCM,
compressed ring ani PSRAM. Błąd API/rozmiaru/wyniku lub brak pamięci
zatrzymuje start przed audio/siecią, raportuje etap; nie przechodzi na
niesprawdzone crypto lub wyłączenie weryfikacji TLS. Ten eksperymentalny
wariant może wtedy wymagać ponownego wgrania przez UART.

Wrapper `--hardware-crypto` jawnie włącza hardware AES/SHA i early init
w prywatnym sdkconfig danego backendu. Przed flash weryfikuje wszystkie
trzy symbole w wygenerowanym nagłówku i dotychczasową etykietę dekodera.
Flagi hardware/software wzajemnie wykluczają się. --software-crypto
wyłącza również early init; wywołanie bez flag zachowuje wybrany profil.
Root config i inne backendy bez zmian. Certyfikaty/hostname/daty/bundle
i zakaz HTTPS downgrade pozostają kontrolowane.

## Odbiór

Na tę próbę przywracamy TCP5760/mailbox6, żeby porównać sprzętowe crypto
z poprzednio przetestowanym software przy tym samym oknie. Wariant23040
z ADR0095 pozostaje dostępny, jego odbiór nie został zgłoszony.

```sh
python3 tools/build_flac.py dr flash --port /dev/ttyUSB0 --hardware-crypto --tcp-window 5760
```

Oczekiwane dwie informacje startupu:

```text
Early crypto: hardware AES/SHA known-answer PASS before I2S; 1024B GDMA probe
TLS crypto: AES=hardware SHA=hardware; certificate verification enabled
```

TCP RX window5760/mailbox6/MSS1440. Publiczny FLAC minimum3min z odsłuchem;
potem STOP/PLAY kilka razy i sekwencja LAN24/headerless→zewnętrzny FLAC→HLS
→FLAC, żeby powtórzyć TLS po zamykaniu I2S. Zachować startup, TLS/UTC,
Delivery/audio_kbps, low/starts, Recover/Reject, CPU/heap. Sam PASS boot
sprawdza wynik przed I2S, nie gwarantuje poprawności kryptografii pod
obciążeniem ani naprawy TLS po I2S. Początkowy brak SNTP i niezaufany root
to osobne przyczyny, które nadal mogą poprawnie odrzucić połączenie.

Testy hostowe wykonują prawdziwy probe z adapterem do OpenSSL, sprawdzają
pełen wynik, no-op,10 trybów awarii/korupcji i zwalnianie zasobów. Test
app_main pilnuje kolejności oraz braku audio/sieci przy błędzie probe.
Testy wrappera sprawdzają oba kierunki wyboru, odrzucenie niezgodnych
artefaktów przed flash, trwałość wyboru i nietknięty root. Wyniki pełnych
testów/native w STATUS:462 host tests PASS; native ESP-IDF6.1 dr/hardware
AES/SHA/early init/TCP5760 build PASS, binarka1573520B. Wrapper potwierdził
trzy symbole i TCP5760/mailbox6; agent nie flashował. Połączenia/odsłuch S3 OPEN.

Jeśli TLS nadal będzie zawodzić, następnym rozdzielającym eksperymentem
może być hardware AES z software SHA. Nie wdrażamy go równocześnie z tą
próbą i nie uznajemy za sprawdzone obejście.
