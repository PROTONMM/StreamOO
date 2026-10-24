# Protokół bramy sprzętowej — HARDWARE_TEST_PENDING

Nie wykonano flashowania, kasowania flash ani operacji eFuse.
Potwierdzenie bazowego uruchomienia N16R8 przekazane przez użytkownika
pozostaje ważne; nie trzeba go powtarzać jako odrębnego etapu.
Poniżej sprawdzamy nowe kontrole startowe oraz niepotwierdzone połączenia.

## Przygotowanie

1. Ustal dokładny model modułu SSD1322 oraz dokumentację producenta.
   Potrzebne: zasilanie modułu, sposób RESET/EN, wybór interfejsu 4-wire SPI,
   mapa COM/SEG/offset kolumn, zalecane parametry inicjalizacji panelu.
   Dokumentacja samego kontrolera nie opisuje podłączenia konkretnego panelu.
2. Przy wyłączonym zasilaniu sprawdź ciągłość połączeń z tabelą
   `02_HARDWARE.md`: OLED 11/12/9/10, RESET bez osobnego GPIO; encoder
   5/16/7; I²S 48/47/21. Potwierdź podciąganie A/B/przycisku na PCB.
3. W terminalu ESP-IDF 6.1 uruchom:

   ```sh
   idf.py menuconfig
   python3 tools/check_config.py sdkconfig --platform
   idf.py reconfigure
   idf.py build
   ```

   W `StreamOO Configuration` zachowaj profil PCB. Opcjonalnie włącz
   `Diagnostic raw encoder GPIO probe`. Podciąganie wewnętrzne włączaj
   zgodnie z potwierdzonym obwodem, bez zgadywania.

## Wgranie i obserwacja — czynność użytkownika

Po sprawdzeniu połączeń użytkownik może uruchomić:

```sh
idf.py -p PORT flash monitor
```

Firmware obecnie nie inicjalizuje OLED ani I²S i nie jest radiem.
Oczekiwane nowe komunikaty:

- `board: N16R8 GPIO map validated`;
- `platform`: IDF 6.1, dwa rdzenie, flash 16777216, PSRAM 8388608;
- metryki pamięci co 10 sekund, bez restartów i błędów startu;
- jeśli włączono probe: `input_probe` z A/B/BTN i liczbą zmian co sekundę.

Obróć enkoder powoli w obie strony, następnie naciśnij i zwolnij przycisk.
Zapisz stany spoczynkowe i wciśniętego przycisku; A i B powinny zmieniać
poziomy. Dryf stanów bez dotykania oznacza potrzebę kontroli podciągania.
Nie porównuj liczby zmian z liczbą detentów — polling może gubić impulsy.
Kierunek, half/full quadrature i debounce nie są tym eksperymentem zatwierdzane.
Wyłącz probe po pomiarach; nie jest docelowym sterownikiem wejścia.

## Wynik do zapisania

- commit firmware, wersja modułu i PCB, opcje menuconfig;
- log rozruchu oraz minimum 30 s metryk pamięci;
- mapa sprawdzonych połączeń, napięcia i dokumentacja resetu OLED;
- poziomy A/B/BTN, obecność podciągania, ewentualne niestabilne sygnały;
- PASS/FAIL/OPEN osobno dla każdej pozycji.

SSD1322: test obrazu/skali 0..15 i czas SPI nadal HARDWARE_TEST_PENDING;
sterownik czeka na identyfikację panelu. Nie zaliczać po samym buildzie.
Audio i testy funkcjonalne gestów pozostają w kolejnych etapach.
