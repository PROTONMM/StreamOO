# 02 — PCB i konfiguracja menuconfig

**USTALONE:** moduł ESP32-S3 N16R8 (16 MB flash, 8 MB PSRAM), wyświetlacz SSD1322 256×64, PCM5102A przez I²S, jeden enkoder. Źródłem połączeń jest `references/YR.kicad_sch`; to konfiguracja PCB YR_0.3.1. Dane z późniejszej analizy schematu mają pierwszeństwo przed starszymi tabelami i definicjami Arduino.

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
| Enkoder #1 | B (S1E2) | 16 | kierunek do potwierdzenia |
| Enkoder #1 | SW (K_E1) | 7 | odczyt schematu |
| Enkoder #2 | A/B/SW | 6/17/18 | fizyczny, nieobsługiwany |

**Weryfikacja:** sygnał CHIP_PU na złączu wyświetlacza jest związany z EN, a nie osobnym GPIO RESET. Nie używaj nieistniejącego resetu sprzętowego; przetestuj sekwencję rozruchu SSD1322. Sprawdź podciąganie enkodera, kierunek i parametry modułu OLED na prawdziwej płytce.

## menuconfig
- Sekcja `StreamOO Configuration`: host SPI, szybkość SPI, mapowanie pinów SSD1322/I²S/enkodera, invert direction, pull-ups, kodowanie kwadraturowe, tryb wejścia A/B, bufor audio, rozmiar FFT, ustawienia startowe.
- GPIO i typy peryferiów: **build-time Kconfig**, użytkowe parametry dynamiczne: **NVS**.
- Walidator: kolizje pinów, odrzucenie niepoprawnych GPIO, sprawdzanie ograniczeń SPI/I²S/PSRAM, zakaz niebezpiecznych domyślnych GPIO 33–37 na N16R8; diagnostyczny komunikat i błąd inicjalizacji.
- Ustal odpowiedni zakres i zasady przypisań po konsultacji dokumentacji modułu; dostępność pinów zależy od konkretnej PCB i funkcji flash/USB/JTAG.

## Test zakończenia
`idf.py menuconfig` umożliwia zmianę wszystkich przypisań; wszystkie ustawienia trafiają do sterowników; kolizja pinów jest wykrywana; test podłączenia na PCB potwierdza sygnały.
