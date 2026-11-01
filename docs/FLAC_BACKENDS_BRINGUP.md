# Porównanie dekoderów FLAC na ESP32-S3

ADR0096 — aktualny test sprzętowego crypto z przygotowaniem przed I2S:

```sh
python3 tools/build_flac.py dr flash --port /dev/ttyUSB0 --hardware-crypto --tcp-window 5760
```

Startup: Early crypto known-answer PASS before I2S, AES/SHA hardware,
TCP5760/mailbox6. Software profil użytkownik ocenił jako mocno przerywający;
skuteczność early init OPEN. FLAC3min z odsłuchem, kilka STOP/PLAY, potem
LAN→zewnętrzny FLAC→HLS→FLAC. Zachować TLS/UTC, low/starts, audio_kbps,
Delivery, Recover/Reject, CPU/heap. Bufory/progi i TLS verification zachowane.
Nie łączymy tej próby ze zmianą okna TCP23040; ADR0095 nadal do odbioru.

ADR0095 — aktualny test: software AES/SHA nie usunęło słyszalnych przerw
publicznego FLAC (logb129d3af,42 low). Sprawdzamy dopływ przy większym
oknie TCP, zachowując dr, ring, PCM i preroll:

```sh
python3 tools/build_flac.py dr flash --port /dev/ttyUSB0 --software-crypto --tcp-window 23040
```

Startup `TCP RX: window=23040 bytes mailbox=18 MSS=1440`. Ta sama publiczna
stacja3min z UART i odsłuchem; zbierz Delivery, Decode buffers/audio_kbps,
Underrun, Recover, CPU oraz heap minimum/largest. Wynik na S3 OPEN.
Profil dotyczy nowych socketów TCP także WWW; większy koszt internal RAM.
Rollback: --tcp-window 5760. Szczegóły i ograniczenia próby hostowej ADR0095.

ADR0093–0094: do odbioru bitrate bez nagłówka (OLED ~) i test TLS po
PSA-149 występującym po I2S. Polecenie dla tego eksperymentu:

```sh
python3 tools/build_flac.py dr flash --port /dev/ttyUSB0 --software-crypto
```

Wrapper kontroluje backend i brak hardware AES/SHA w wygenerowanej
konfiguracji przed flash. Startup ma pokazać AES=software SHA=software.
Dotyczy wybranego sdkconfig.flac-dr; root i inne backendy bez zmian.
Walidacja certyfikatów pozostaje. LAN→zewnętrzny FLAC→HLS→FLAC;
wpływ na połączenia/CPU/CRC OPEN. Szczegóły ADR0094.

ADR0092: kontrolowany LAN ma0 low dla16bit/~55s i24bit/~78s oraz1 low
headerless/~165s przy pełnym źródle, bez CRC/empty. Nowy firmware rozkłada
okresowe logi na pojedyncze linie między uzupełnianiem PCM. Odtworzona
regresja w modelu UART20ms/linia poprawiona; to nie deklaracja naprawy
publicznego live. Odbiór: LAN24/headerless po3min, potem publiczny FLAC.
Delivery ma teraz log_step_max_us, nie max czasu całego pakietu.

ADR0091: PS_NONE potwierdzony na b2431e5, ale50 low/1 Recover w~3min.
Kolejny test używa znanych plików LAN, bez zmiany firmware. Gotowe media
i CSV w build/flac-lan-20261101, procedura [FLAC_LAN_TEST](FLAC_LAN_TEST.md).
FLAC16, ciężki24bit oraz identyczne24bit bez nagłówka pliku pozwalają
rozdzielać podstawową ścieżkę od bootstrap live i publicznego transportu.

ADR0090: test bf40138/dr ma97 low, HTTP read do674ms, log batch do82,8ms;
Wi-Fi raportuje pm type1. Kolejny build wyłącza modem sleep, bez zmiany
buforów/TCP/dekodera. Sprawdź `Wi-Fi power save=NONE; low-latency RX`,
potem3–5min tej samej stacji z Delivery/Underrun/Recover. Źródło CRC i wpływ
nowego ustawienia OPEN; szczegółowe wyniki/ograniczenia w ADR0090.

ADR0089: log5ad29183 nadal ma82 Underrun i2 Recover bez reconnect gen3.
Użytkownik nie miał możliwości odsłuchu. Nowy build mierzy dopływ i czasy
HTTP/body oraz okresowego logowania: `player: Delivery`. Ten sam dr i stacja,
3–5min logu od resetu. Nie zmieniono buforów ani progów; szczegółowe pola
i ograniczenia w ADR0089. Brak OLED UI i nagłówka wersji w starym załączniku
nie potwierdza uruchomienia optymalizacji6b90399.

ADR0086: log ae2a00bd ujawnił frame_limit_no_boundary z niezerowym CRC
po128KiB. Dodano fallback do kolejnej całej ramki z CRC8+CRC16, dopiero
przy limicie lub błędnym EOS; wpis Recover oznacza pominięte audio.
To nie naprawa źródła błędnych danych ani zwykłych niedoborów PCM.
Nowa próba z tym samym dr ma porównać Recover/Reject i Underrun oraz odsłuch.

ADR0085: log85dc8a81 potwierdził dr_flac-0.13.4 na S3 (firmware73fa94e).
121 ostrzeżeń Underrun i3 błędy parsera adaptera -1001, mimo aktywnego
ReaderESP_OK/HTTP200. CPU0/1 średnio41,36%/43,98%; bez dowodu stałego
nasycenia. Odsłuch niepełny, stabilność nieodebrana. Dodano dokładny
log flac_parse: status/reason/bytes/scan/crc16 i16 bajtów prefiksu na błędzie.
Następny test pozostaje na dr; nie zmieniono buforów ani progów.

ADR0081: wrapper korzysta z Pythona wskazanego przez IDF_PYTHON_ENV_PATH
lub środowiska pasującego do wersji SDK, nawet jeśli samo python3 wskazuje
inny venv/system. Nie rozwiązuje symlinku Pythona do /usr/bin. Po zmianie
interpretera zachowuje stary katalog kompilacji i tworzy świeży. Narzędzia
SDK nadal wymagają eksportu środowiska:

```sh
. "$IDF_PATH/export.sh"
python3 tools/build_flac.py dr flash --port /dev/ttyUSB0
```

Pierwsza linia wrappera `ESP-IDF Python: ...` powinna wskazywać venv SDK.

ADR0080 koryguje polecenia wgrywania: samo `-B build/flac-dr` nie wybiera
backendu. Logc7f8d36f i ELF2262539b9 z tego katalogu nadal miały micro.
Do porównań używaj sprawdzającego skryptu:

```sh
python3 tools/build_flac.py dr build
python3 tools/build_flac.py dr flash --port /dev/ttyUSB0
```

Analogicznie `micro` lub `esp`. Każde wywołanie jawnie podaje SDKCONFIG,
a konfiguracja sdkconfig.flac-dr/micro/esp jest poza build/ i nie trafia do
repo. Skrypt sprawdza wygenerowany nagłówek oraz etykietę backendu w binarce
przed flash. Katalogi flac-dr/micro/esp mają też blokadę CMake na inny backend.
Zwykłe menuconfig dla domyślnego build nadal działa jak poniżej; nie należy
traktować samej nazwy katalogu jako wyboru konfiguracji.

Wynik micro-flac z logu5c378192: zapas CPU, ale błędy LPC/CRC i niedobory;
stabilność nieodebrana. Wynik dr i następny pomiar opisano powyżej.
Szczegóły pomiarów micro i granice wniosków w ADR0079.

ADR0078. Dostępne trzy osobne warianty firmware. Wybór przy kompilacji,
bez zmiany playlisty ani NVS. Domyślnie micro-flac; tylko jeden backend FLAC
jest używany w danym buildzie. MP3/AAC/Vorbis/Opus pozostają w Espressif.

## Wybór

```sh
idf.py menuconfig
```

`StreamOO → Audio and DSP → FLAC decoder for comparison tests`:

- Espressif2.6.2 z parserem przyrostowym;
- micro-flac0.3.0, domyślny;
- dr_flac0.13.4 z adapterem pełnych ramek.

Potem zwykłe `idf.py build` i `idf.py -p PORT flash`, bez erase.
Używaj tego samego katalogu i jawnie tej samej konfiguracji w menuconfig/
build/flash. Dla porównawczych katalogów korzystaj ze skryptu powyżej.
Zachowaj log i ELF każdego wariantu. Rozstrzygający wpis startowy:
`decoder: FLAC backend=...`.
Sama zmiana sdkconfig.defaults nie nadpisuje istniejącego wyboru w sdkconfig.
Wybór nie jest przełącznikiem WWW ani automatycznym fallbackiem po błędzie.

## Co porównujemy

| Wariant | Wejście i pamięć | Wyjście do wspólnego pump |
|---|---|---|
| Espressif | cache SDK i własny parser CRC z ADR0077 | natywny PCM16/24/32 |
| micro-flac | rzeczywiste przyrostowe API, stan subramek/Rice, workspace PSRAM-only; domyślnie bez przechowywania dodatkowych metadanych | PCM32, MSB-aligned |
| dr_flac | własny cache około132KiB PSRAM, CRC parser i jedna kompletna ramka na odczyt, jeden trwały handle dr_flac; alokacje dekodera w PSRAM | PCM32, MSB-aligned |

dr_flac używa API pull: krótki odczyt/zero może oznaczać EOF. Dlatego nie
podajemy mu bezpośrednio źródła DATA/WAIT. Adapter zbiera kompletną ramkę,
podaje ją z pamięci i odbiera dokładnie jej liczbę próbek. Nie blokuje sieci,
nie odtwarza uchwytu na każdą ramkę i nie używa prywatnych pól bitstreamu.
Nagłówek STREAMINFO zachowany, kolejne ramki korzystają z tego samego dekodera.
Ostatni bajt wejścia z EOS pozostaje pending do opróżnienia wcześniejszych
ramek cache; zapobiega zgubieniu końcówki pliku w istniejącym pull pump.
Ta dodatkowa warstwa oznacza, że mierzymy cały adapter, a nie same kernele.

Wszystkie warianty mają ten sam transport HTTP/ICY, ring256KiB PSRAM,
preroll/refill FLAC~500ms, scratch1MiB PSRAM, FIFO160ms RAM wewnętrzny,
resampling48kHz, gain i I2S. Nie zwiększono buforów równocześnie z wymianą
dekodera. Bootstrap dla live bez fLaC pozostaje wspólny. Ogg FLAC wyłączone
w obu nowych bibliotekach; test dotyczy native FLAC. Format końcowej ramki
może mieć1..15 próbek zgodnie z RFC9639; reader granic już ich nie odrzuca.
Sam bootstrap live nadal wymaga normalnej samopiszącej ramki.

## Protokół sprzętowy

1. Najlepiej identyczny plik FLAC24-bit z lokalnego serwera LAN, z dokładnie
   tym samym PCM. `tools/generate_extended_fixtures.py` przygotowuje sygnały
   lokalne; publiczna stacja z różnymi utworami nie daje kontrolowanego A/B.
2. Każdy wariant: minimum5min widmo FFT, minimum1min bargraph, potem powrót;
   przy publicznym TEST FLAC native lossless objąć również zmianę utworu.
   Ten sam poziom głośności, widget, Wi-Fi, źródło, czas i zegary240MHz/PSRAM80MHz.
3. Sprawdzić STOP/PLAY, przejście na MP3/AAC i powrót, reset, prawidłowe tempo,
   stereo oraz brak słyszalnych przerw. Nie kasować NVS ani LittleFS.
4. Zapisać UART od startu. W tabeli wyników podać backend, źródło i czas,
   CPU0/1, task player/audio_levels/http_reader, low/hard/starts i liczbę retry.

Logi:

- `CPU interval`/`CPU task`: rzeczywiste statystyki RTOS co10s po warm-up;
  load tasku to procent jednego rdzenia. Player obejmuje też resampling,
  przygotowanie bloków i zarządzanie odtwarzaniem, nie sam kodek.
- `Decode perf`: backend, maksymalny czas wywołania/bloku, czas sumaryczny,
  liczba wywołań i średni czas wywołania; są to czasy ścienne, zawierają
  wywłaszczenia. Avg_call nie jest porównywalny bez uwzględnienia różnych
  rozmiarów/progresu wywołań; dla CPU korzystać z profilera RTOS.
- `Decode buffers` i `Decode flow`: największy wynik PCM, FIFO przed/po
  najdłuższym wywołaniu, WAIT, budget_yield, pobrane bajty i wyprodukowany PCM.
  PCM32 alternatyw daje więcej bajtów niż PCM24 Espressif przy tym samym audio.
- `Buffer interval`: min/max compressed w bajtach i PCM w blokach240 ramek
  (jeden blok5ms). Obserwacje ownera od poprzedniego logu, nie dokładne
  minima ISR; licznik low/hard pozostaje właściwym dowodem niedoborów.
- `Underrun`: empty/busy, pending, cache PCM, stan refill. Liczniki
  kumulacyjne w sesji; porównywać przyrosty i generation.
- `Reader`/`Stream failed` oraz wpisy Wi-Fi: oddzielić błąd dekodera od
  przyczyny sieciowej. Błąd dekodera powoduje zamknięcie sesji przez player;
  samo ponowne łączenie nie dowodzi, że wcześniej zerwał się transport.
- `heap internal`/`PSRAM` oraz stack_free/HWM: zapas, minimum i największy
  ciągły blok. Nie utożsamiać większej ilości wolnej pamięci z mniejszym CPU.

## SIMD

micro-flac: zlinkowane restore_lpc_32bit_asm/64bit_asm, skalarne mull/mulsh
i pętle sprzętowe; przejrzany kod źródłowy i ścieżki wywołań nie używają
wektorów PIE. To optymalizacja Xtensa, nie SIMD. dr_flac ma upstream SSE/NEON,
bez backendu Xtensa; w adapterze SIMD wyłączone jawnie, na S3 ścieżka C.
FFT nadal ma niezależny wektorowy backend ESP-DSP.

## Stan weryfikacji

Build native ESP-IDF6.1 dla wszystkich trzech wariantów PASS.409 testów
hostowych PASS, w tym6 z realnymi bibliotekami: PCM identyczny z FFmpeg,
cisza, ton16, szum24, końcowe7 próbek, join po pierwszej ramce, porcje
1/7/257/4096/65536B, reset i CRC uszkodzony. ASan/UBSan alternatyw6 i
aktora player11 PASS (LSan off). Dodatkowo178 pełnych ramek starego capture
tej samej stacji: oba backendy i reset oddają bajtowo zgodny PCM z FFmpeg.

Pomiary CPU/pamięci S3: pierwszy micro w ADR0079 i dr w ADR0085.
Kontrolowane A/B, płynność oraz pełny odbiór S3 nadal OPEN.
Hostowe czasy nie stanowią pomiaru wydajności na Xtensa. Nie deklarować,
że wymiana biblioteki rozwiązała wszystkie niedobory sieci lub PCM.

ADR0079 rozszerza próby hostowe o micro32 (logika bitreader/CLZ32 przy
ABI64), deterministycznie zmienne porcje1..4096B i ponowne użycie wejścia.
Nie zmienia oryginalnych plików upstream ani nie testuje ASM Xtensa.
Także starszy capture178 pełnych ramek przechodzi te warianty; nie jest
to odtworzenie dokładnej ramki wywołującej nowy błąd LPC/CRC.
