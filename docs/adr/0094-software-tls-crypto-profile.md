# ADR0094 — test programowego AES/SHA po I2S

Data: 2026-11-01. Status: profil do odbioru S3; przyczyna TLS OPEN.

## Nowy log

f02f1a01-7050-49f4-9eed-914a226362d2, firmware90958c2/ELF4b53d51f,
SHA256 `fa7d48a59430ebf2e7a09a4c776818e7a964e0cf465b8d1a7787b7d4018ce40d`.
LAN headerless do148,888s:0 Underrun, końcowe low0/hard0/starts1,
log_step_max9922us. Pozytywny ograniczony wynik ADR0092, bez nowej
deklaracji odsłuchu ani zaliczenia soak.

Po LAN zewnętrzny FLAC gen3..10 nie przechodzi TLS. HLS HE-AACv2 gen12..16
i MP3 gen18 podobnie:14 błędów PSA-149 / podpisu certyfikatu i handshake
-0x3000, HTTP0. Gen19 osobno nie dopasowuje root. Gen20 HLS MP3 w końcu
gra; publiczny FLAC gen22 też, ale ma12 low i1 Recover w548,078s
(skipped9744/validated20898/CRC1d41) do końca757s. To odrębne zjawiska.

Lokalny SDK6.1 definiuje PSA_ERROR_INVALID_SIGNATURE=-149; bundle zgłasza
go po psa_verify_hash. AES/SHA w badanym buildzie były sprzętowe. Późnych
błędów nie wyjaśniamy tylko czasem przed SNTP. Ponieważ linia Reader w
załączniku urywa się przed TLS, dodano krótką osobną linię: error/code,
flags/errno, UTC/time_synced. Bez haseł, URL i query.

## Wskazówka i eksperyment

[Zgłoszenie autora w ESP-IDF #18640](https://github.com/espressif/esp-idf/issues/18640)
opisuje PSA-149 po I2S na S3 oraz poprawę przy programowym AES/SHA.
Maintainer proponował reset GDMA; zgłoszenie zamknięto10.10.2026 z braku
odpowiedzi. To wskazówka, nie potwierdzony upstream fix ani rozpoznana
przyczyna StreamOO. Nie kopiowano kodu zgłoszenia.

Sprawdzono lokalny SDK6.1: port AES/SHA korzysta ze współdzielonego GDMA
alokowanego przy pierwszym użyciu, Kconfig udostępnia software fallback.
Istniejąca zależność SDK/API Apache2.0, bez nowej biblioteki, własnej
kryptografii lub modyfikowania globalnego SDK.

```sh
python3 tools/build_flac.py dr flash --port /dev/ttyUSB0 --software-crypto
```

Flaga ustawia tylko CONFIG_MBEDTLS_HARDWARE_AES/SHA na n w prywatnym
sdkconfig wybranego backendu. Wrapper po build sprawdza wygenerowany
nagłówek; niezgodność blokuje flash. Inne ustawienia, root sdkconfig,
pozostałe backendy oraz kontrola etykiety binarki zachowane.
Startup: `TLS crypto: AES=software SHA=software`.

Podpisy certyfikatów, hostname, daty, CA bundle i zakaz downgrade nadal
sprawdzane. Wi-Fi ROM, MPI/RSA i FFT SIMD pozostają. Większy koszt CPU
TLS wymaga pomiaru S3. To nie import ROOT CA ani wyjątek HTTPS. Następne
zwykłe wywołanie wrappera zachowuje konfigurację danego backendu; warto
powtarzać flagę, aby wymusić również kontrolę przed flash. Akcelerację
można przywrócić przez menuconfig, poza tym eksperymentem.

## Odbiór

Testy wrappera: idempotencja, zachowanie ustawień, odrzucenie niezgodnego
sdkconfig.h, brak flash po błędzie i nietknięty root sdkconfig. Pełne
wyniki host/native build w STATUS; startup/config nie zalicza TLS na S3.

Powtórzyć LAN FLAC→zewnętrzny FLAC→HLS→FLAC. Zachować wersję, startup
crypto, krótkie TLS/UTC/flags, CPU i niedobory. Zniknięcie błędów będzie
wynikiem eksperymentu, nie automatycznym potwierdzeniem wady GDMA.
Osobno obserwować CRC i dopływ audio. Agent nie flashował urządzenia.
