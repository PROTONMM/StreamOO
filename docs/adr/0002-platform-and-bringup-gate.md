# ADR 0002 — platforma 6.1 i brama testów PCB

Data: 2026-10-24. Status: przyjęte dla części bazowej etapu 03.

## Decyzja

Wymagana gałąź ESP-IDF 6.1 i target esp32s3 są kontrolowane przez CMake.
Zweryfikowano v6.1, commit `fff9895c82d744c7237be8847347bdd1b07c6643`.
Aktualizacja patch wymaga ponownego buildu i testów; kodeki/ESP-DSP nie są
jeszcze wybrane ani dodane. Lokalny sdkconfig nie trafia do nowych commitów.
Walidator konfiguracji wymaga 16 MB flash, Octal PSRAM 80 MHz, malloc,
testu PSRAM i CPU 240 MHz. Nie zmieniono zastanych GPIO ani częstotliwości.

Komponent `platform` sprawdza model/liczbę rdzeni, rozmiar flash oraz
zainicjalizowane 8 MiB PSRAM. Błąd zatrzymuje uruchamianie peryferiów.
Raportuje ilość i największy blok pamięci wewnętrznej/PSRAM oraz minimum
heap. Osobne zadanie priorytetu 1 co 10 sekund raportuje heap i zapas stosu.
Nie przypisujemy rdzenia bez pomiarów. Alokacja stosu jest tylko przy starcie.
Nie ma jeszcze ścieżki audio, jej latencja i wpływ sieci pozostają OPEN.
CPU w logu oznacza profil kompilacji, nie pomiar chwilowej częstotliwości.

Opcjonalny `hardware_probe` (`STREAMOO_INPUT_PROBE`, domyślnie wyłączony)
ustawia tylko A/B/BTN jako wejścia z podciąganiem zgodnym z menuconfig.
Próbkuje co tick RTOS, raportuje poziomy i liczbę zaobserwowanych zmian
co sekundę. To eksperyment elektryczny: nie jest dekoderem kwadraturowym,
może gubić impulsy, nie interpretuje kierunku ani gestów.

## Sprawdzone zależności

Wyłącznie lokalne komponenty ESP-IDF 6.1, Apache-2.0:
`esp_psram` (`esp_psram_is_initialized`, `esp_psram_get_size`),
`spi_flash` (`esp_flash_get_size`), `esp_hw_support` (`esp_chip_info`),
`heap` (metryki `heap_caps_*`), `esp_driver_gpio` (`gpio_config`,
`gpio_get_level`), `freertos` (zadania, tick). API sprawdzono w nagłówkach
zainstalowanego IDF; nie skopiowano implementacji bibliotek.

## Brama dalszych prac

Użytkownik potwierdził wcześniejszy boot, flash, CPU i test PSRAM. Nie jest to
potwierdzenie połączeń SSD1322 ani działania nowego kodu diagnostycznego.
Repozytorium jawnie nie określa modelu panelu OLED i nie rozstrzyga jego
zasilania/resetu. Dokumentacja kontrolera SSD1322 określa komendy, lecz nie
ustala okablowania COM/SEG, offsetu kolumn i parametrów analogowych modułu.
Nie wybieramy ich na podstawie obcej biblioteki ani podobnego panelu.

Przed inicjalizacją OLED wymagane: identyfikacja modułu/datasheet i
potwierdzenie resetu/zasilania na PCB. Następnie etap 04a: transport SPI,
profil panelu i test skali szarości. Bez tego nie można zaliczyć sprzętowej
bramy etapu 02 ani pełnego etapu 03. Procedura: `docs/HARDWARE_BRINGUP.md`.

Źródło komend do przyszłego eksperymentu (bez importu kodu):
[SSD1322 datasheet, Solomon Systech rev. 1.2](https://crystalfontz.com/controllers/uploaded/Solomon_Systech_SSD1322_Datasheet_v1.2_July_2010.pdf).
