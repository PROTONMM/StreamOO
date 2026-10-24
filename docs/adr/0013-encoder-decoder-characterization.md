# ADR 0013 — dekoder enkodera i eksperyment GPIO

Data: 2026-10-24. Status: przyjęty dla etapu 05a; odbiór sprzętowy OPEN.

Własny komponent input zawiera czystą maszynę stanów C, bez Arduino,
alokacji, GPIO, RTOS i zależności UI. Nie skopiowano kodu zewnętrznego.
Adapter hardware_probe używa obecnego ESP-IDF 6.1: esp_driver_gpio (Apache-2.0),
esp_timer i FreeRTOS z zainstalowanego SDK; bez nowych pakietów.

Odczyt A/B: tablica legalnych przejść Gray, cofnięcia znoszą się,
skok dwóch bitów porzuca niepełny krok. Pełny krok kończy się na 11 po
4 przejściach; półkrok na 00/11 po 2. Kierunek odwracany konfiguracją.
Nie zakładamy jeszcze, że wybrany półkrok odpowiada jednemu ząbkowi sprzętu.
Przycisk aktywny niski: debounce 20ms, long >=600ms liczony między
zaakceptowanymi zboczami, emisja po zwolnieniu. Obrót podczas naciśnięcia
(także kandydującego przed debounce) tłumi późniejszy klik. Start z trzymanym
przyciskiem wymaga zwolnienia przed uzbrojeniem klików.

Nowy Kconfig STREAMOO_ENCODER_GESTURE_TEST domyślnie włącza diagnostykę
w konsoli. Zastępuje dawny przełącznik surowego probe. GPIO i pull-up bez
zmian. Próbowanie raz na tick RTOS i logowanie NIE gwarantują uchwycenia
szybkich zboczy; to eksperyment wolnych ruchów, nie docelowy transport wejść.
Następny krok wymaga przechwytywania zboczy z obsługą przepełnień i pomiarów
szybkiego obracania przed mapowaniem do sterowania. UI ma nadal jednego właściciela;
probe nie wysyła komend i nie rysuje.

NVS, tryby gestów A/B, sterowanie playerem i regulacja volume są poza 05a.
