# ADR0029 — przyszłe sterowanie RC5 i aplikacją telefonu

Data:2026-10-31. Status: wymagania ustalone; implementacja przyszła/OPEN.

## Decyzja użytkownika

W przyszłości StreamOO obsłuży pilot RC5 i aplikację telefoniczną sterującą
radiem przez API. To rozszerzenie zakresu, nie żądanie wdrożenia obu funkcji
w aktualnym kroku konfiguracji sieci.

## Granice architektury

Enkoder, adapter RC5, WWW i klient telefonu kierują zweryfikowane komendy
do wspólnego właściciela odtwarzacza. Nie tworzą osobnych stanów playera
ani nie sterują bezpośrednio dekoderem/DMA. Wejścia sprzętowe mają własne
adaptery i kolejki; dekodowanie IR nie trafia do ścieżki próbek I2S.

Aplikacja telefonu korzysta z wersjonowanego API StreamOO, wspólnego z WWW.
DTO pozostają oddzielone od struktur sterowników, a zmiany stanu mają
walidację, uprawnienia i odczyt aktualnego stanu z tego samego ownera.
Nie kopiujemy API yoRadio. Istniejący GET/api/v1/status jest tylko pierwszym
odczytem; endpointy sterowania nie zostały jeszcze zaimplementowane.

Reguły trybu AP obowiązują wszystkie źródła komend: nie wolno uruchomić
odtwarzania podczas konfiguracji przez RC5 ani telefon. MicroSD pozostaje
osobnym przyszłym źródłem; ewentualne wyjątki wymagają osobnej decyzji.

## Otwarte przed implementacją

RC5: model odbiornika, połączenie/GPIO ze schematu i potwierdzonego sprzętu,
mapa pilota, powtarzanie/przytrzymanie klawiszy, zachowanie w menu i biblioteka
z przeglądem licencji/API ESP-IDF. Nie przypisujemy teraz pinów ani kodów.

Telefon: docelowe platformy i forma aplikacji, sposób dostępu/wykrywania
urządzenia, uwierzytelnianie/uprawnienia, zakres akcji i aktualizacja stanu.
Wersjonowany kontrakt sterowania wymaga dopracowania; tabela w10_WEB jest
propozycją, nie gotowym kontraktem dla aplikacji mobilnej.

Zapis wyłącznie dokumentacji; bez nowego kodu, buildów i testów sprzętu.
Bieżący krok pozostaje AP bez audio, następnie konfiguracja STA/AP off po IP.
