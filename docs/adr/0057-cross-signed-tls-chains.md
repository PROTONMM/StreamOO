# ADR0057 — poprawne łańcuchy certyfikatów cross-signed

Data:2026-10-31. Status: konfiguracja poprawiona, handshake S3 OPEN.

Log użytkownika dla PL Nowy Swiat - AAC+: No matching trusted root,
TLS handshake-0x3000, HTTP0, stage=http_end/ESP_ERR_HTTP_CONNECT. Dekoder
nie został uruchomiony. Nie jest to potwierdzenie błędu HE-AAC.

## Diagnoza

Odczytany publiczny chain stream.nowyswiat.online:
leaf nowyswiat.online → GoogleWE1 → GTS Root R4 cross-signed przez dawny
GlobalSign Root CA. Przypięty pełny Mozilla bundle zawiera samopodpisany
GTS Root R4, nie zawiera dokładnego starego GlobalSign Root CA. Domyślny
callback bez cross-sign próbuje dopasować issuer końca podanej ścieżki,
co odpowiada obserwowanemu No matching trusted root.

OpenSSL z CAfile DOKŁADNIE z przypiętego ESP-IDF6.1 poprawnie buduje
ścieżkę leaf→WE1→zaufany GTS Root R4. Sam zestaw CA nie wymaga w tym
przypadku uzupełniania. Kolejne hosty stream.rcs.revma.com oraz n01-eu1
mają chain Let's EncryptYR2→RootYR cross-signed→ISRG RootX1; sprawdzone
z tym samym CAfile. Odczyt z hosta jest dowodem dla jego trasy DNS/TLS,
nie przechwyceniem handshake urządzenia.

## Zmiana

Włączamy natywne CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_CROSS_SIGNED_VERIFY.
Kconfig wybiera MBEDTLS_X509_TRUSTED_CERT_CALLBACK. Włączone w defaults
oraz bieżącym sdkconfig; dla starszego sdkconfig ustawić przez menuconfig
→ Component config → mbedTLS → Certificate Bundle → cross-signed verify.

Dodatkowo audyt wykazał CONFIG_MBEDTLS_HAVE_TIME_DATE=n w poprzednim
sdkconfig (domyślny wybór SDK). Włączono y: radio ma już SNTP. Wcześniejszy
opis sugerujący pełne sprawdzanie dat w firmware był nieprecyzyjny.
To osobna korekta profilu, nie przyczyna komunikatu No matching root.
Przed uzyskaniem poprawnego czasu HTTPS może wymagać ponownej próby.

Bez wymiany SDK/kodu dostawcy, nowych CA, dodawania leaf/intermediate
jako trust anchor, deprecated roots, wyłączenia hostname/date/signature
lub przejścia do HTTP. Full Mozilla bundle i klient crt_bundle_attach
pozostają. Daty są teraz jawnie włączone. Callback SDK sprawdza alternatywną ścieżkę do już zaufanego
klucza. Lokalny kod SDK rozróżnia syntetyczny root bundle od realnych
certyfikatów przy sprawdzaniu czasu. Nie usuwamy sprawdzania realnej
ważności certyfikatów. Udokumentowany przez SDK narzut około700B heap
nie jest naszym pomiarem na S3; realny HWM/heap po flash OPEN.

## Weryfikacja

- Odczyt publicznych chainów trzech hostów, brak sekretów.
- OpenSSL+przypięty CAfile: poprawny hostname PASS; błędna nazwa i czas
  po wygaśnięciu leaf dają FAIL. To hostowy test kryptograficzny, nie
  uruchomienie callbacka Mbed TLS ESP32.
- Regresje rzeczywistego readera HTTP (17 testów z atrapą transportu),
  GPIO/config validator, diff check i native build według STATUS.
- Przeczytano Kconfig/API i istniejący test dostawcy cross-signed z czasem;
  testu sprzętowego ESP-IDF nie uruchomiono bez płytki.

Odbiór: zwykły flash bez erase/importu, ponownie ten sam RNŚAAC+.
Oczekiwane Certificate validated, HTTP200 i następnie PCM/PLAYING. Jeśli
następnie wystąpi błąd dekodera, analizować go oddzielnie. Poprawka FLAC
ADR0056 pozostaje w tym samym buildzie, nadal wymaga odsłuchu S3.

Źródła: przypięty SDK6.1 components/mbedtls/Kconfig, esp_crt_bundle.c,
test_apps/mbedtls_ut/main/test_esp_crt_bundle.c; oficjalny opis:
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_crt_bundle.html
