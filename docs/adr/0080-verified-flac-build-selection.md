# ADR0080 — pewny wybór konfiguracji i identyfikacja backendu FLAC

Data:2026-11-01. Status: trzy buildy/sprawdzenie binarek PASS; dr odsłuch S3 OPEN.

## Dowód błędnego polecenia

Użytkownik wkleił logc7f8d36f po instrukcji `idf.py -B build/flac-dr flash`.
Log wskazuje micro-flac0.3.0. Aktualny ELF z build/flac-dr ma SHA256
2262539b9503a83978bc0f7efe2e5daab47e23613d5433d918b58fefb284c455,
zgodny z prefiksem2262539b9 w logu. Wygenerowane sdkconfig.h również ma
CONFIG_STREAMOO_FLAC_MICRO, a ELF restore_lpc_32bit_asm zamiast dekodera dr.
Poprzedni build/flac_dr.sdkconfig i pozostałe katalogi porównawcze zniknęły.
Nie ustalono dokładnego polecenia czyszczenia/rekonfiguracji użytkownika.

Wcześniejsza instrukcja agenta była niewystarczająca: nazwa katalogu -B
nie wybiera opcji Kconfig. Przygotowana wcześniej konfiguracja nie jest
gwarancją po późniejszym czyszczeniu lub ponownej konfiguracji. Wnioski
o pierwszym logu micro pozostają, ale tego drugiego logu nie można zaliczyć
jako odbioru dr_flac. Sam App version nie identyfikuje backendu.

## Decyzja

tools/build_flac.py micro|dr|esp build|flash --port PORT:

- Osobne ignorowane sdkconfig.flac-* w głównym katalogu, poza build/.
  Pierwszy wybór kopiuje pozostałe ustawienia z lokalnego sdkconfig, później
  zachowuje ustawienia danego wariantu. Nie nadpisuje głównego sdkconfig.
- Każdy etap build ORAZ flash dostaje jawne -D SDKCONFIG=pełna_ścieżka.
- Po build sprawdza dokładnie jeden wybór w config/sdkconfig.h i dokładnie
  jedną oczekiwaną etykietę backendu w streamoo.bin; raportuje rozmiar i SHA.
  Przy rozbieżności flash nie jest uruchamiany. Brak automatycznego flash
  po samym build; agent nie uruchamia flash.
- CMake blokuje katalogi o udokumentowanych nazwach flac-dr/flac-micro/
  flac-esp przy innym backendzie, także po usunięciu całego katalogu build.
  Niezależny zwykły katalog build i ręczne menuconfig nadal działają.

## Porównanie lekkości

Nowe buildy IDF6.1 z tym samym lokalnym profilem; konfiguracja i binarki
sprawdzone skryptem, nie tylko po nazwie katalogu:

| Backend | Cały firmware bajty | KiB |
|---|---:|---:|
| micro-flac |1552720|1516.33|
| dr_flac |1566544|1529.83|
| Espressif |1567248|1530.52|

micro ma najmniejszy flash w tym profilu: o13.5KiB mniej niż dr i14.19KiB
mniej niż Espressif. To rozmiar całej aplikacji, nie samodzielnej biblioteki.
W obecnej integracji dr ma dodatkowy cache wejścia132KiB+16B (FRAME_CACHE),
którego micro nie wymaga dzięki API przyrostowemu. Wszystkie warianty nadal
rezerwują wspólny scratch1MiB i ring compressed256KiB. Pełna pamięć runtime
i CPU dr na fizycznym S3 nadal niezmierzone; nie wybieramy zwycięzcy CPU
na podstawie benchmarku hosta lub nieporównywalnych utworów.

## Nowy log micro i kierunek diagnozy

Gen3 działa od około12.5s do końca logu182s, bez błędu dekodera. Pięć
Underrun do21.395s; potem brak kolejnych w zapisie. Końcowy ring compressed
około250KiB, PCM32. CPU17 próbek: średnio CPU0 41.76%,CPU1 36.10%,
player30.45%. Maksymalny codec16.895ms, blok34.664ms. Wcześniejsze dwie
próby TLS nieudane jeszcze przed audio. To nie dowodzi stałego przeciążenia
CPU ani wspólnej awarii dwóch bibliotek, bo test nadal używa micro.
Niedobory dopływu/buforowania zależne od przebiegu pozostają istotnym
kierunkiem, a błędy LPC/CRC z poprzedniego logu nadal wymagają wyjaśnienia.

## Weryfikacja

414 testów hostowych PASS, w tym5 testów wyboru, niejednoznacznego nagłówka,
niezgodnej binarki i zablokowania flash po złym buildzie. Wszystkie3 warianty
zbudowane i zweryfikowane nagłówkiem+etykietą binarki PASS. Próba konfiguracji
osobnego /tmp/.../flac-dr z głównym sdkconfig micro kończy się oczekiwanym
FATAL_ERROR Wrong FLAC backend przed kompilacją/flash. Walidatory GPIO i
diff PASS. Zmiana nie modyfikuje dekodera, transportu, I2S ani buforów.
Nie wykonano flash/erase/eFuse, zmian NVS/playlisty ani odsłuchu dr na S3.
