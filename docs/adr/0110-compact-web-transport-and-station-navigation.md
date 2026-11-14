# ADR0110 — mniejsze sterowanie WWW i większa lista stacji

Data:2026-11-14. Status:wdrożone; fizyczny telefon/ESP32 OPEN.

Po obejrzeniu ADR0109 użytkownik wymaga usunięcia informacji o Wi-Fi,
powtarzanych etykiet edytora, mniejszych zakładek i sterowania ikonami,
przycisków poprzedniej/następnej stacji oraz około2/3 ekranu na listę.

## Decyzja

Usunięto kartę sieci/IP/AP i komunikat poprawnego połączenia z głównego
widoku. Błędy połączenia i instrukcje przejścia AP→LAN nadal są pokazywane.
Mobilny nagłówek ma30px, zakładki28px. Nazwa stacji i status w jednym
wierszu, kodek/bitrate/próbkowanie w kolejnym. Długą nazwę skraca CSS;
pełny tekst pozostaje w DOM dla technologii asystujących.

Sterowanie to jeden rząd: poprzednia, PLAY, STOP, następna, suwak i wartość
głośności. Cztery własne ikony SVG, bez tekstu wizualnego, z aria-label/title.
Przyciski36×36px; mniejszy rozmiar jest świadomą realizacją uwagi użytkownika.
Pole wyszukiwania, licznik i odświeżanie tworzą pojedynczy pasek nad listą.
Lista zajmuje co najmniej66% wysokości okna w testach320×740 i390×844;
krótszy ekran poziomy może przewijać stronę zamiast ucinać kontrolki.

Poprzednia/następna używa pełnego zapisanego katalogu, niezależnie od filtra,
z zawijaniem na obu końcach. Bez aktywnego ID: następna wybiera pierwszą,
poprzednia ostatnią. Mniej niż2 stacje, AP/offline, zapis katalogu lub
trwające żądanie blokują nawigację. Wybór z listy i ikon korzysta z tej samej
funkcji i istniejącego POST/stations/select z generacją katalogu/CAS.
Nie zmieniono API ani obsługi firmware. Wybór stacji rozpoczyna odtwarzanie
zgodnie z dotychczasowym kontraktem, również po STOP.

Wspólna flaga żądania chroni przed podwójnym wysłaniem. Po przyjęciu wyboru
kolejna nawigacja bazuje na ostatnim przyjętym celu, bez czekania na polling.
Oczekujące ID odróżniają pośredni status wcześniejszego polecenia od zmiany
zewnętrznej. Nazwa/zaznaczenie nadal pochodzą z potwierdzonego statusu.
Przełączenie AP/katalog bez celu usuwa oczekujący stan; błędy nie przesuwają
celu nawigacji. Istniejący configGeneration odrzuca starszy odczyt statusu.

Wiersze edytora mają ukryte wizualnie etykiety nazwy/URL i mały przycisk×
z opisem dla czytnika ekranu. Na PC pola obok siebie, na telefonie dwa
zwarte wiersze i usuwanie z prawej. Formularz dodania nadal ma etykiety.

## Weryfikacja

- 23 istniejące testy portalu PASS; node --check i git diff --check PASS.
- check_portal_browser.py: AP/LAN, 4 kolejne kroki nawigacji w obu kierunkach,
  zawijanie i aktywny filtr, podwójny klik podczas żądania, blokada dla
  pojedynczej/pustej playlisty, PLAY/STOP/volume, edytor/import/CAS oraz
  Wi-Fi/zegar/motywy PASS w lokalnym Chromium390px.
- check_portal_layout.py: pięć konfiguracji320..1280px i dodatkowy podgląd
  edytora390px PASS. Sprawdzono udział listy>=66% wysokości w pionie,
  brak scrolla całej strony odtwarzacza, nazwy dostępne ikon, ukryte etykiety,
  2048 stacji, filtr, zachowanie przewijania i motywy. PNG obejrzane.
- Native ESP-IDF6.1 build PASS:0x176a80B,90% partycji app wolne.

Bez nowych zależności, zmian DSP/I2S i flash agenta. Ocena dotyku36px na
fizycznym Androidzie oraz przełączania rzeczywistych stacji pozostaje OPEN.
