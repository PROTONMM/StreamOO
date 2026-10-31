# ADR0052 — nazwa mDNS odtwarzacza w LAN

Data: 2026-10-31. Status: wdrożone; odbiór multicast na sprzęcie OPEN.

Użytkownik potwierdził działanie trybów WWW i edytora z ADR0051. Wygląd
strony pozostaje do późniejszego dopracowania. Kolejny krok etapu09:
dostęp pod `http://streamoo-xxxxxx.local/`, gdzie sufiks jest tym samym
MAC-derived identyfikatorem co AP, zapisanym małymi literami.

## Integracja

Osobny adapter discovery, wywoływany wyłącznie przez owner sieci, uruchamia
mDNS po uzyskaniu STA/IP i wyłączeniu AP. Ogłasza `_http._tcp` na porcie80,
instance StreamOO, bez danych stacji, SSID routera ani sekretów w TXT.
SDK obsługuje multicast i konflikty nazwy. Callback wypisuje aktualny adres
po zmianie nazwy (w tym automatycznym rozwiązaniu konfliktu); callback nie
wywołuje API mDNS. Początkowy log oznacza start, nie potwierdzenie zdalnego
rozwiązania nazwy. Po utracie STA lub wejściu w AP owner zwalnia usługę;
ponowne połączenie uruchamia ją ponownie. SDK reaguje na zdarzenia IP także
podczas blokującej próby kolejnych profili. Nie dodano pracy w ścieżce I2S.

Błąd dowolnego kroku inicjalizacji zwalnia rozpoczęty responder; retry
najwcześniej po30s, dostęp po IP pozostaje. Nie ma dodatkowego zadania
aplikacji, jedynie zadanie SDK priority1/4096B stosu wewnętrznego. Dynamiczne
alokacje mDNS w PSRAM; wyłączone browse, CLI, multiple-instance i responder
AP/Ethernet. Jeden service. Maxinterfaces2: próba1 ujawniła błąd upstream
1.14.0 (`mdns_pcb.c`, inicjalizator dwóch elementów); używamy2 bez modyfikacji
źródeł dostawcy. Faktycznie aktywny wyłącznie interfejs STA.

## Zależność i ograniczenia

Dokładnie espressif/mdns1.14.0, hash w dependencies.lock. Manifest wymaga
IDF>=5.0; zweryfikowano native build na przypiętym6.1. Publiczne mdns.h
SPDX Apache-2.0 i pobrany LICENSE Apache2 sprawdzone przed integracją.
Źródła dostarcza Component Manager. Zachowany Apache-2.0.txt i notice.
Starsze cache1.8.2 pominięto: nowsze wersje naprawiają parser DNS i granice
buforów. Źródło: oficjalny tag mdns-v1.14.0 repo esp-protocols.

Nazwa konfigurowalna przez WWW/NVS pozostaje osobnym krokiem. Ten etap
nie dodaje nowego UI. Multicast wymaga wsparcia klienta/routera; izolacja
klientów/VLAN może uniemożliwić .local, wówczas pozostaje adres IP z OLED.
Rzeczywisty narzut heap, konflikty, reconnect i audio z multicast do odbioru
na płytce. Testy hostowe sprawdzają adapter i błędy, nie stos sieciowy SDK.
