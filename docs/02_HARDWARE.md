# 02 — PCB i konfiguracja menuconfig

Praktyczna instrukcja ustawiania GPIO i pozostałych opcji:
[BUILD_GUIDE.md](BUILD_GUIDE.md#pierwsza-konfiguracja-i-menuconfig).

**USTALONE:** moduł ESP32-S3 N16R8 (16 MB flash, 8 MB PSRAM), wyświetlacz SSD1322 256×64, PCM5102A przez I²S, jeden enkoder. Źródłem połączeń jest `references/YR.kicad_sch`; to konfiguracja PCB YR_0.3.1. Dane z późniejszej analizy schematu mają pierwszeństwo przed starszymi tabelami i definicjami Arduino. Wyjątek potwierdzony na sprzęcie przez użytkownika 2026-10-24: enkoder A/B/BTN = 5/6/7; schemat referencyjny ma B na 16.

| Urządzenie | Sygnał | GPIO | Status |
|---|---|---:|---|
| SSD1322 | MOSI | 11 | odczyt schematu |
| SSD1322 | SCLK | 12 | odczyt schematu |
| SSD1322 | CS | 9 | odczyt schematu |
| SSD1322 | DC | 10 | odczyt schematu |
| SSD1322 | RESET | -1 | nie przypisano programowego resetu |
| PCM5102A | I²S DOUT | 48 | odczyt schematu |
| PCM5102A | I²S BCLK | 47 | odczyt schematu |
| PCM5102A | I²S LRCLK | 21 | odczyt schematu |
| Enkoder #1 | A (S1E1) | 5 | kierunek do potwierdzenia |
| Enkoder #1 | B | 6 | połączenie potwierdzone przez użytkownika; schemat S1E2→16 nie odpowiada temu sprzętowi |
| Enkoder #1 | SW (K_E1) | 7 | odczyt schematu |
| Enkoder #2 w schemacie | A/B/SW | 6/17/18 | nieobsługiwany; GPIO6 zajęte przez B używanego enkodera |

**Weryfikacja:** sygnał CHIP_PU na złączu wyświetlacza jest związany z EN, a nie osobnym GPIO RESET. Nie używaj nieistniejącego resetu sprzętowego; przetestuj sekwencję rozruchu SSD1322. Sprawdź podciąganie enkodera, kierunek i parametry modułu OLED na prawdziwej płytce.

## menuconfig
- Sekcja `StreamOO Configuration`: host SPI, szybkość SPI, mapowanie pinów SSD1322/I²S/enkodera, invert direction, pull-ups, kodowanie kwadraturowe, tryb wejścia A/B, bufor audio, rozmiar FFT, ustawienia startowe.
- GPIO i typy peryferiów: **build-time Kconfig**, użytkowe parametry dynamiczne: **NVS**.
- Walidator: kolizje pinów, odrzucenie niepoprawnych GPIO, sprawdzanie ograniczeń SPI/I²S/PSRAM, zakaz niebezpiecznych domyślnych GPIO 33–37 na N16R8; diagnostyczny komunikat i błąd inicjalizacji.
- Ustal odpowiedni zakres i zasady przypisań po konsultacji dokumentacji modułu; dostępność pinów zależy od konkretnej PCB i funkcji flash/USB/JTAG.

## Test zakończenia
`idf.py menuconfig` umożliwia zmianę wszystkich przypisań; wszystkie ustawienia trafiają do sterowników; kolizja pinów jest wykrywana; test podłączenia na PCB potwierdza sygnały.

## Implementacja etapu 02 (2026-10-24)

`board_config` oraz `tools/check_config.py` walidują mapę przed użyciem GPIO.
CMake odrzuca błędną konfigurację; firmware ma niezależną kontrolę startową.
Polityka rezerwacji i źródła: [ADR 0001](adr/0001-n16r8-gpio-policy.md).
Host SPI jest konfigurowalny (2/3), domyślnie SPI2; USB19/20 zarezerwowane.
Zmiana GPIO nie świadczy o dostępności sygnału na złączu konkretnej PCB.
Odbiór połączeń, resetu i zasilania OLED nadal HARDWARE_TEST_PENDING.

## Zidentyfikowany OLED (2026-10-24)

Moduł użytkownika: msalamon 3,12 cala OLED_312_YELLOW, 256×64, SSD1322.
Użytkownik potwierdził R5/R8, odpowiadające BS1=0/BS0=0 (4-wire SPI).
Nie zmieniono GPIO ani RESET=-1. Dane źródłowe, sprzeczność podpisów zdjęć
sprzedawcy i profil eksperymentalny opisuje [ADR 0003](adr/0003-ssd1322-msalamon-spi-bringup.md).
Test połączeń, zasilania/resetu i obrazu nadal HARDWARE_TEST_PENDING.
