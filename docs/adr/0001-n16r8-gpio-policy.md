# ADR 0001 — walidacja mapy GPIO przed inicjalizacją

Data: 2026-10-24. Status: przyjęte dla etapu 02.

## Kontekst

Kconfig dopuszcza numery 0..48, ale ESP32-S3 nie posiada GPIO22..25.
N16R8 zajmuje linie pamięci; RESET=-1 oznacza brak GPIO, nie GPIO EN.
Sama kontrola zakresu nie chroni przed kolizją wyjść z pamięcią lub konsolą.

## Decyzja

Komponent `board_config` ma czystą funkcję C oraz adapter ESP-IDF.
Adapter sprawdza wszystkie 11 sygnałów z menuconfig przed inicjalizacją
peryferiów. Odrzuca GPIO22..25, GPIO26..37 (flash/Octal PSRAM), kolizje
łącznie z RESET oraz GPIO0/3/45/46 jako konserwatywną politykę pinów
strapujących. Ta ostatnia reguła jest polityką projektu, nie ograniczeniem
wszystkich zastosowań układu. GPIO19/20 są rezerwowane domyślnie oraz
zawsze, gdy używa ich konsola USB. Konsola UART0 rezerwuje GPIO43/44;
niestandardowe TX/RX są uwzględnione w kontroli kolizji. Zewnętrzny JTAG
na GPIO39..42 wymaga ręcznej rezerwacji w profilu instalacji (OPEN).

Hostowy walidator uruchamiany przy CMake i firmware mają test zgodności
2332 konfiguracji; kod C jest kompilowany na hoście z `-Werror`.
Host SPI wybiera się w menuconfig między SPI2/SPI3; SPI0/1 są niedostępne.
Profil domyślny i lokalny sdkconfig zachowują dotychczasowe przypisania.

## Źródła i zależności

Nie dodano zależności zewnętrznych. API log/esp_err i Kconfig pochodzą z
ESP-IDF v6.1 (Apache-2.0), commit
`fff9895c82d744c7237be8847347bdd1b07c6643`.
Sprawdzono lokalne `soc/esp32s3/include/soc/soc_caps.h` oraz
[oficjalne ograniczenia GPIO](https://github.com/espressif/esp-idf/blob/v6.1/docs/en/api-reference/peripherals/gpio/esp32s3.inc).
Źródłem profilu pozostają `references/YR.kicad_sch` i `docs/02_HARDWARE.md`.

## Konsekwencje

Błąd konfiguracji przerywa CMake, a niezależnie firmware pozostawia
peryferia wyłączone. Testy hostowe nie potwierdzają połączeń PCB ani
zasilania/resetu OLED. Odbiór elektryczny: HARDWARE_TEST_PENDING.
