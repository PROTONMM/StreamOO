# ADR 0016 — adapter gestów i interaktywny test UI

Data: 2026-10-24. Status: przyjęty, odbiór interaktywny na sprzęcie OPEN.

Czysty adapter `streamoo_ui_handle_input` w komponencie ui przyjmuje jawne
zdarzenie input i tryb A/B. Na HOME obrót emituje VolumeDelta, klik
zależny od trybu otwiera listę albo emituje Toggle, pressed-rotate cyklicznie
zmienia jeden z trzech widżetów. Na liście zwykły obrót przesuwa kursor,
short zatwierdza, long anuluje; pressed-rotate jest ignorowany. Taki obrót
nie jest zwykłym przewijaniem ani regulacją volume. Pozycja listy pozostaje
clamped, D19 nadal OPEN. Puste potwierdzenie i odrzucona komenda nie zamykają listy.

Volume i odtwarzanie należą do właściciela playera, nie do UI. Callback
sterowania ma skopiować komendę nieblokująco; brak miejsca oznacza false.
Istniejący callback SelectStation zachowuje stable_id i catalog_generation.
Adapter nie dodaje pól stanu playera do ui i nie wykonuje operacji NVS.
Wizualizacja jest stanem UI. Jedna delta to jeden krok volume w demonstracji;
nie określa to krzywej wzmocnienia PCM.

hardware_probe nadal sam dekoduje wejścia. Nowa kolejka 32 gestów kopiuje
zdarzenia do pojedynczego odbiorcy w zadaniu ekranu. Pierwszy receive włącza
przekazywanie; wcześniej zdarzenia są tylko logowane. Pełna kolejka odrzuca
nowy gest i loguje ostrzeżenie. Nie jest to gwarancja bezstratności ani
produkcyjny transport przy przeciążeniu. Oddzielna kolejka 128 próbek IRQ
zachowuje dotychczasową diagnostykę overflow. ISR i zadanie wejścia nie
mutują UI. Nie dodano bibliotek: używamy istniejącego FreeRTOS z ESP-IDF 6.1.

Domyślny nowy STREAMOO_ENCODER_UI_TEST, zależny od testu wejść i OLED,
uruchamia interaktywny HOME od razu zamiast skończonego pokazu 46s.
Wyłączenie opcji przywraca stary pokaz. Ekran pracuje do resetu lub błędu.
Aplikacyjny encoder_demo ma własny jawny model TEST: volume 50, PLAY,
stacja 1002, statyczny katalog pięciu stacji i syntetyczne widżety. Callbacki
kopiują komendy, a testowy właściciel stosuje je po powrocie z adaptera.
Volume jest ograniczone do 0..100; wybór stacji zmienia testowy snapshot
z nową generacją. Browse/cancel nie zmieniają testowego źródła. STOP zeruje
syntetyczne poziomy, bez wpływu na prawdziwy I2S. Wiersz TEST VOL/PLAY/W
jest częścią aplikacyjnej demonstracji, nie rendererem produkcyjnego HOME.

Brak rzeczywistego audio, playera, sieci, czasu i zapisu preferencji.
NVS po 5s oraz obsługa shutdown pozostają etapem magazynu/integracji.
Pełnego etapu 05 nie uznajemy za ukończony przed odbiorem gestów i trwałości.
