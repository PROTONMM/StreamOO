# 01 — Cel i zakres

**USTALONE:** Nowy produkt **StreamOO** jest internetowym radiem z ESP32-S3 N16R8, DAC PCM5102A, SSD1322 256×64 i jednym enkoderem. Firmware od początku w natywnym ESP-IDF; bez Arduino Core. Logikę użytkową rekonstruujemy na podstawie yoRadio, lecz nowy projekt nie dziedziczy interfejsów sieciowych, architektury ani błędów implementacji źródła.

## Cele użytkowe
- Odtwarzaj radiowe strumienie HTTP/HTTPS, przełączaj stacje bez blokowania UI.
- Obsługuj podstawowe MP3/AAC oraz docelowo FLAC, Ogg Vorbis, Opus (stan kodeków potwierdzić testami).
- Wyświetlaj stację, utwór, dodatkowy tekst, informacje o kodeku/bitrate, RSSI Wi-Fi, datę, czas i wizualizację.
- Zarządzaj listą stacji i konfiguracją sieci z WWW; import/export format zgodny z yoRadio.
- Pozwól na warianty A/B gestów, własne GPIO przez menuconfig, profil YR PCB 0.3.1.
- Pracuj niezawodnie po chwilowej utracie Wi-Fi/strumienia; auto retry bez natrętnego resetowania sprzętu.

## Granice
- Bez kopiowania kodu lub API z yoRadio; przed użyciem biblioteki należy sprawdzić licencję i wsparcie ESP-IDF.
- Animacja i UI są funkcjonalnymi wymaganiami, natomiast efekty graficzne trzeba dostroić po testach OLED.
- Nie przewiduje się dwóch enkoderów w bieżącej wersji firmware mimo istnienia złącza na PCB.

## Test zakończenia etapu
Istnieje identyfikowalny katalog wymagań z testami akceptacyjnymi; każda niezdefiniowana cecha jest wymieniona w `13_DECISIONS_OPEN.md`.
