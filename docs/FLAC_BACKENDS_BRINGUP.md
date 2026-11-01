# Porównanie dekoderów FLAC na ESP32-S3

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

Aktualny wynik micro-flac z logu5c378192: zapas CPU, ale błędy LPC/CRC
i niedobory; stabilność nieodebrana. Następny wariant do odsłuchu: dr_flac.
Szczegóły pomiarów i granice wniosków w ADR0079.

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

CPU, płynność, zajętość pamięci i odbiór fizycznego S3 nadal OPEN.
Hostowe czasy nie stanowią pomiaru wydajności na Xtensa. Nie deklarować,
że wymiana biblioteki rozwiązała wszystkie niedobory sieci lub PCM.

ADR0079 rozszerza próby hostowe o micro32 (logika bitreader/CLZ32 przy
ABI64), deterministycznie zmienne porcje1..4096B i ponowne użycie wejścia.
Nie zmienia oryginalnych plików upstream ani nie testuje ASM Xtensa.
Także starszy capture178 pełnych ramek przechodzi te warianty; nie jest
to odtworzenie dokładnej ramki wywołującej nowy błąd LPC/CRC.
