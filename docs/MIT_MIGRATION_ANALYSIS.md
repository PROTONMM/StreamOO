# MIT: analiza wymian i pełna lista modułów

## Aktualny stan — MIT nadane / ADR0112 / 2026-11-14

Na polecenie użytkownika własny kod StreamOO otrzymał standardową MIT.
Właściciel wskazany przez użytkownika: Mariusz Księżak. Obowiązują
[LICENSE](../LICENSE), [zakres i wyjątki](../LICENSING.md) oraz
[noty zależności](../THIRD_PARTY_NOTICES.md). Mały font pozostaje public domain,
Krypton OFL, kodeki Espressif na swoich ograniczonych warunkach.
Dołączono brakujące kopie not SDK/runtime z rejestrem pochodzenia i hashami.

Poniższa analiza jest historyczna. Jej wzmianki o nienadanej licencji własnej
aplikacji zastępuje ADR0112; licencje materiałów zewnętrznych pozostają odrębne.

## Aktualizacja fontu — ADR0102 / 2026-11-14

Bieżący mały font to public-domain Misc Fixed Unicode5x8; zastąpiono też jego sześć klatek wzorcowych. Poniższe zalecenia wymiany fontu UNKNOWN są historyczne i dla bieżących bitmap zostały wykonane przez wybór public domain. Inwentarz CSV uaktualniono w tych dwóch pozycjach; nie wykonano migracji pozostałych zależności ani nadania MIT.

Data: 2026-11-01. Analiza na żądanie użytkownika, bez zmiany firmware/licencji.
Stan kodu: poprawka PCM47e44c5, następnie dokumentacyjny27ad8d0.

## 1. Co oznacza cel „StreamOO na MIT”

**Można wydać własne źródła StreamOO na MIT, zachowując inne licencje
bibliotek, SDK i fontów. Nie trzeba w tym celu wymieniać Apache/BSD/OFL
na MIT.** Obowiązki zależności muszą pozostać w dystrybucji. Przykładowy opis:
„StreamOO application: MIT; third-party components: see notices”.
Nie jest to nadanie licencji obecnym plikom ani ustalenie właściciela praw.

Rozważamy trzy odrębne cele:

| Cel | Co trzeba zrobić |
|---|---|
| A. Własna aplikacja na MIT, zależności na swoich warunkach | Wyjaśnić/wymienić mały font PL, ustalić prawa do własnego kodu, nadać MIT z wyjątkami i dołączyć noty zależności. Kodek Espressif można zachować dla ESP32. |
| B. Jak A, dodatkowo usunąć ograniczenie kodeków do produktów Espressif | Wymienić cały używany tor esp_audio_codec i jego parsery; Apache/BSD/OFL/MPL mogą pozostać. Inne platformy nadal wymagają portu SDK. |
| C. Każdy składnik dystrybuowanej aplikacji/obrazu wyłącznie na dokładnie MIT | Znacznie więcej niż wymiana kodeków: SDK, TLS, IP, storage, DSP, fonty, CA i runtime. Nie ma tu zweryfikowanej kompletnej drogi zachowującej wszystkie funkcje ESP32-S3. |

MIT-0, BSD i Apache nie są tekstem ani identyfikatorem SPDX MIT, mimo że
są liberalnymi licencjami. Sama zgodność z MIT lub możliwość sublicencji
nie pozwala usuwać obowiązkowych not upstream. OFL i MPL dotyczą wskazanych
fontów/danych/plików, a nie automatycznie całej własnej aplikacji.
[MIT](https://opensource.org/license/mit),
[Apache2.0 §4](https://www.apache.org/licenses/LICENSE-2.0),
[OFL FAQ1.3](https://openfontlicense.org/ofl-faq/),
[MPL FAQ](https://www.mozilla.org/en-US/MPL/2.0/FAQ/).

## 2. Zakres i kompletność spisu

Sprawdzono wszystkie27 katalogów komponentów StreamOO oraz main,
manifesty, licencje lokalne, użycie API i lokalną mapę linkera zwykłego
buildu. ELF SHA256:
`5fa63ece873f82a3237f0829d073a3ac5795a93c21ea31eeb2685461c2fdf001`.
To ten sam prefiks ELF5fa63ece, który użytkownik potwierdził na płytce.

Pełny spis archiwów, modułów i zasobów:
[MIT_MODULE_INVENTORY.csv](MIT_MODULE_INVENTORY.csv),114 rekordów,
w tym103 archiwa z niezerową sekcją mapowaną do pamięci aplikacji.
Każde takie archiwum występuje w spisie; biblioteki wymienione tylko jako
LOAD lub discarded nie są liczone jako włączone do wynikowego kodu.
`pcm_adapter` jest w projekcie, lecz w tym snapshotcie nie wnosi własnego
archiwum do obrazu; jego publiczne typy są używane przez inne moduły.

To inwentarz na poziomie modułów/archiwów i decyzji migracyjnych, nie
certyfikowany SBOM z ustalonymi prawami do każdego bajtu bibliotek binarnych.
Licencja główna SDK nie zastępuje nagłówków jego plików i not zależności.
Modyfikacja konfiguracji/kodeków zmienia skład linkowanego obrazu, więc
przed faktycznym wydaniem trzeba odświeżyć spis dla wybranego profilu.
Osobny bootloader/ROM/toolchain nie jest automatycznie objęty MIT aplikacji.

## 3. Wymiany bibliotek i zasobów

„Zostaje” w kolumnie A oznacza zachowanie własnej licencji i not upstream,
nie przepisanie nagłówka biblioteki na MIT.

| Moduł/zależność | Obecne warunki | Cel A: własna aplikacja MIT | Cel B/C: zmiana lub ograniczenie |
|---|---|---|---|
| esp_audio_codec2.6.2 i esp_audio_simple_dec | Espressif Modified MIT | Zostaje na ESP32, z ograniczeniem producenta | Cel B wymaga pełnej wymiany dekoderów i parserów poniżej; nie wystarczy dr_flac |
| dr_flac0.13.4 | Wybrana MIT-0 | Zostaje | Już usuwa zależność FLAC od vendorowego dekodera. MIT-0 ma inny identyfikator niż MIT; ustalić politykę wyjątków przy celu C |
| micro-flac0.3.0, opcjonalny | Apache-2.0 | Zostaje jako alternatywa | Dla C usunąć wariant/źródła z wydania i zachować dr_flac lub inny uzgodniony backend |
| ESP-DSP1.8.2 | Apache-2.0 | Zostaje, również SIMD | Dla C własny FFT lub MIT; utrzymanie wektorowego sc16 wymaga osobnej implementacji i testów |
| cJSON1.7.19~2 | MIT | Zostaje | Brak potrzeby wymiany |
| HTTP parser2.7.0 | MIT | Zostaje | Brak potrzeby wymiany; port SDK oceniany osobno |
| LittleFS adapter1.20.4 | MIT | Zostaje | Sam adapter może pozostać, ale nie zmienia licencji rdzenia |
| LittleFS core | BSD-3-Clause | Zostaje z notą BSD | Dla C inny filesystem MIT albo własny format magazynu i port flash |
| mDNS1.14.0 | Apache-2.0 | Zostaje | Dla C własny responder DNS-SD/mDNS lub sprawdzona implementacja MIT |
| esp_http_client/server, esp-tls, tcp_transport | Głównie Apache-2.0 SDK | Zostają | Dla C wymiana klienta/serwera/transportu; nasz HTML/JS/REST może pozostać |
| Mbed TLS4.1.0, TF-PSA-Crypto | Wybrana Apache-2.0 z alternatywy Apache/GPL | Zostają | Dla C inny stos TLS/X509/crypto z właściwymi warunkami i portem S3; zmiana backendu sprzętowego i crypto prepare |
| Mozilla CA bundle | MPL-2.0 | Zostaje z notą i odpowiadającym źródłem | Dla C odrębnie uzyskany zbiór zaufania z ustalonymi prawami; skopiowanie podzbioru nie nadaje MIT |
| FreeRTOS kernel | MIT | Zostaje | Nie wymieniać samego jądra MIT; część portu/integracji IDF ma odrębne noty |
| lwIP / wpa_supplicant | BSD + integracja SDK | Zostają z notami | Dla C inny stos IP/supplicant MIT i integracja; duży zakres |
| heap/TLSF | Apache-2.0 + BSD-3-Clause | Zostaje | Dla C inny allocator i port internal/PSRAM/DMA |
| Pozostały ESP-IDF, sterowniki, bootloader, Wi-Fi/PHY | Głównie Apache-2.0 i wyjątki per-file | Zostają na swoich warunkach | Dla C wymiana istotnej części platformy; brak zweryfikowanego pełnego MIT zamiennika Wi-Fi S3 |
| libgcc / libstdc++ runtime | GPL z GCC Runtime Library Exception3.1 | Może pozostać z uwzględnieniem wyjątku | Nie jest zwykłą MIT; dla C alternatywny runtime, nie samo wybranie innego kompilatora |
| Picolibc/libc/libm | Rodzina BSD/MIT, noty per-file | Zostaje z właściwymi notami | Dla C sprawdzić każdą używaną część lub wymienić runtime; nie jest to jednorodny moduł MIT |
| Mała czcionka5x8 ASCII/PL | Baza Adafruit BSD, modyfikacje PL UNKNOWN | Wyjaśnić prawa lub wymienić | Nowe oryginalne bitmapy ASCII i PL z ustalonym MIT; zregenerować obrazy referencyjne |
| Monaspace Krypton / StreamOO UI16 | OFL-1.1 | Zostaje jako font OFL | Dla C zastąpić fontem MIT/własnym; skalowanie/rasteryzacja nie usuwa OFL z danych fontu |
| Oryginalne logo / HTML / CSS / JS | Własne pliki; fonty osobno | Mogą otrzymać MIT po ustaleniu praw | Nie trzeba przepisywać oryginalnej grafiki/frontendu |
| references/ i materiały yoRadio | Materiały odniesienia, prawa osobno | Nie obejmować automatycznie MIT | Odrębnie licencjonować albo wyłączyć z publikowanej paczki |

Najważniejsza restrykcja producenta jest jawna w
[licencji dokładnie2.6.2](https://components.espressif.com/components/espressif/esp_audio_codec/versions/2.6.2/license).
ESP-IDF sam opisuje swoje mieszane warunki w
[Copyrights and Licenses](https://github.com/espressif/esp-idf/blob/master/docs/en/COPYRIGHT.rst).
Kernel FreeRTOS, lwIP, wpa, heap i porty sprawdzono także w lokalnym SDK6.1.

GCC Runtime Library Exception pozwala na użycie wskazanego runtime
przez niezależny program nie-GPL, również komercyjny, przy spełnieniu
warunków wyjątku. Nie należy tego mylić z linkowaniem zwykłej biblioteki
GPL bez wyjątku. Licencje samego GCC/FFmpeg/CMake jako narzędzi hostowych
nie przechodzą automatycznie na ich wynik; dołączony runtime rozpatrujemy
oddzielnie. Źródło: [GCC COPYING.RUNTIME](https://github.com/gcc-mirror/gcc/blob/master/COPYING.RUNTIME),
lokalne TOOLCHAIN/share/licenses/gcc/COPYING.RUNTIME i COPYING3.
Picolibc opisuje warunki plikowo w
[COPYING.picolibc](https://github.com/picolibc/picolibc/blob/main/COPYING.picolibc).
Skrypt hostowy AGPL wymieniony w całym pakiecie Picolibc nie jest dowodem,
że AGPL kod został włączony do naszego libc/firmware.

## 4. Jak usunąć esp_audio_codec bez utraty funkcji

Należy wymienić **obie** biblioteki libesp_audio_codec.a i
libesp_audio_simple_dec.a oraz korzystające z nich adaptery. Zewnętrzne
biblioteki są na razie kandydatami: nie zostały pobrane do repo, przypięte
nowym hashem, skompilowane ani zmierzone na S3 w ramach tej analizy.

| Funkcja | Aktualnie | Konkretny kierunek | Koszt i ograniczenia |
|---|---|---|---|
| MP3 | esp_mp3_dec + parser simple | dr_mp3, MIT-0, lub minimp3 z osobną weryfikacją wersji | Średni: adapter strumieniowy, dowolne porcje wejścia, resync, mono/stereo, backpressure, budżet RAM/CPU. Brak potwierdzenia SIMD Xtensa |
| AAC-LC | esp_aac_dec / ADTS | Kandydat do celu B: libxaac Apache-2.0; cel C nie ma tu zweryfikowanego pełnego MIT zamiennika | Wysoki: port, profile, RAM, 44.1/48kHz, ADTS, reset i deadline na S3 wymagają PoC |
| HE-AAC/AAC+ v1 i v2 | Ten sam vendorowy AAC, SBR/PS enabled | Ten sam problem wyboru biblioteki co AAC-LC | Nie traktować samego dekodera AAC-LC jako zastępstwa SBR/PS. Deklaracje upstream nie są odbiorem S3 |
| FLAC | Domyślnie dr_flac, alternatywy micro/vendor | Zachować istniejący dr_flac0.13.4/MIT-0 | Najmniejszy koszt: usunąć opcje vendor/micro jeśli niepotrzebne; już mamy adapter i testy. To samo nie usuwa reszty esp_audio_codec |
| Ogg Vorbis | esp_vorbis_dec + esp_ogg_dec | stb_vorbis, wybrać MIT | Średni/wysoki: push streaming, przydziały i chained Ogg. Upstream opisuje ograniczenia łączenia strumieni; sprawdzić zmiany logicznego strumienia na stacjach |
| Ogg Opus | esp_opus_dec + esp_ogg_dec | libopus BSD-3-Clause dla celu B, własny Ogg demux | Średni/wysoki: port fixed-point i budżety. Rozwiązuje restrykcję Espressif, ale nie daje dosłownie MIT. Nie wskazano kompletnego sprawdzonego Opus MIT |
| Ogg container | esp_ogg_dec/simple parser | Parser w stb_vorbis dla Vorbis; własny parser Ogg dla Opus lub libogg BSD | Osobna obsługa nagłówków, page/packet/lacing, CRC, granule, serial i BOS/EOS. Wybór samego libopus nie wystarcza |
| MPEG-TS w HLS | esp_ts_dec | Własny bounded demux TS/PAT/PMT/PES albo sprawdzony MIT demuxer | Średni/wysoki:188B pakiety, continuity, PTS, podzielone PES/ADTS/MP3 i zmiany segmentów. MP3/AAC dekoder sam nie zastąpi TS |
| HLS manifesty i pobieranie segmentów | Własny streamoo_hls / http stream | Zachować własny kod | Migracji wymaga TS i decoder dispatch, nie parser M3U8 jako taki. Nadal ten sam ograniczony profil HLS |
| M3U/PLS i ICY | Własne resolvery/filtr metadanych | Zachować | Własny kod może mieć MIT; nowy dekoder musi zachować kontrakt WAIT/EOF/consumed/produced |

Źródła kandydatów i ich rzeczywiste warunki:
[dr_mp3](https://github.com/mackron/dr_libs/blob/master/dr_mp3.h),
[dr_flac](https://github.com/mackron/dr_libs/blob/master/dr_flac.h),
[stb_vorbis](https://github.com/nothings/stb/blob/master/stb_vorbis.c),
[stb MIT option](https://github.com/nothings/stb/blob/master/LICENSE),
[libopus COPYING](https://github.com/xiph/opus/blob/main/COPYING),
[libxaac LICENSE](https://github.com/ittiam-systems/libxaac/blob/main/LICENSE),
[libxaac opis/porty](https://github.com/ittiam-systems/libxaac/blob/main/README.md).

Pułapki AAC: [FAAD2](https://github.com/knik0/faad2/blob/master/COPYING)
ma GPL lub osobną ścieżkę komercyjną, [FDK AAC](https://github.com/mstorsjo/fdk-aac/blob/master/NOTICE)
ma odrębne warunki Fraunhofer, a [Helix AAC](https://github.com/pschatzmann/arduino-libhelix/blob/main/src/libhelix-aac/aacdec.c)
zachowuje RPSL/RCSL w kodzie dekodera. Żaden nie staje się MIT tylko
dzięki nazwie repo/wrappera. To analiza źródłowej noty Helix, nie propozycja
użycia Arduino Core. Wymiana biblioteki nie jest też audytem praw patentowych
formatów audio; takiego audytu tutaj nie przeprowadzono.

## 5. FFT i zachowanie wektoryzacji

Dla A/B **pozostawić ESP-DSP**: Apache-2.0 nie blokuje MIT własnego DSP/UI.
Własny kod liczy Hann/pasma/envelope; vendor dostarcza szybki sc16 AES3.

Dla C mamy już własny portable-float-radix2, wybierany bez flagi SIMD,
ale trzeba także usunąć zależność ESP-DSP z CMake/manifestu. Tracimy wtedy
obecny backend wektorowy, zmienia się budżet CPU i numeryka FFT. To nie
jest równoważny zamiennik wydajnościowy bez pomiarów. Zachowanie SIMD
wymaga własnego niezależnego kernela/zgody na dodatkową licencję albo
innej odpowiedniej implementacji.

[muFFT](https://github.com/Themaister/muFFT/blob/master/COPYING) jest przykładem
biblioteki MIT, lecz jej opisane SSE/AVX nie są SIMD ESP32-S3. Nie wskazuję
jej jako gotowego zamiennika AES3; testowe programy muFFT mają również
odrębne zależności FFTW, których nie należy linkować do firmware.

## 6. Wszystkie własne moduły —27 komponentów oraz main

„Zachować” zakłada oryginalność i prawa do własnego kodu. Nie nadaje MIT
wskazanym osobno fontom, vendored źródłom ani bibliotekom SDK.

| Moduł | Funkcja | Działanie licencyjne/techniczne |
|---|---|---|
| `audio_gain` | Głośność i rampy | Zachować; własny kod |
| `audio_output` | DMA/I2S i callback IRAM | Zachować adapter; sterownik I2S/SDK ma odrębne warunki |
| `board_config` | GPIO i walidacja N16R8 | Zachować; zależy od definicji platformy SDK |
| `clock` | Strefy, zegar, ustawienia czasu | Zachować; NVS/SNTP/system czasu osobno |
| `compressed_source` | Źródło bajtów i ring/backpressure | Zachować; zależy od FreeRTOS |
| `configuration` | CSV/JSON, snapshot katalogu, LittleFS/NVS | Zachować; cJSON MIT, rdzeń LittleFS BSD i NVS SDK osobno |
| `decoder` | Adaptery, FLAC bootstrap/parser, codec dispatch | Wymienić punkty esp_audio_simple_dec/codec dla celu B; własne parsery i dr_flac zachować |
| `graphics` | Canvas4bpp, clipping, prymitywy | Zachować; własny kod |
| `hardware_probe` | Funkcjonalny driver enkodera/IRQ i opcjonalne logi | Zachować; historyczna nazwa nie oznacza zewnętrznej biblioteki |
| `input` | Dekoder kwadratury i gesty | Zachować; własny kod |
| `levels` | RMS, peak, ballistics | Zachować; libc/math ma osobne warunki |
| `main` | Składanie komponentów i opt-in testy sprzętowe | Zachować; po migracji zmienić manifest/linkowanie; testy ocenić razem z klipami/fontami |
| `metering` | Zadanie analizy i tap PCM | Zachować; zależności DSP i FreeRTOS osobno |
| `network` | STA/AP, reconnect, SNTP, mDNS | Zachować logikę; Wi-Fi/IP/SDK/mDNS pozostają zależnościami |
| `pcm` | SPSC FIFO i funkcjonalny tor próbek | Zachować; własny kod |
| `pcm_adapter` | Typy PCM i dawny adapter klipu | Zachować; brak własnego wkładu archiwum w tym ELF |
| `platform` | N16R8, diagnostyka i early crypto | Zachować logikę; IDF/PSA/AES/SHA mają odrębne warunki |
| `player` | Jedyny właściciel sesji/komend/buforów | Zachować; aktualizować adapter dekodera bez zmiany kontraktu |
| `portal` | Własny REST, HTML/CSS/JS | Zachować; cJSON MIT, serwer SDK osobno |
| `preferences` | Odroczony zapis volume/widget | Zachować; NVS/RTOS/timer osobno |
| `settings` | Ustawienia systemu i ostatnia stacja | Zachować; NVS/RTOS osobno |
| `spectrum` | Hann, FFT, pasma, stream/envelope | Własny kod zachować; ESP-DSP tylko do wymiany przy celu C |
| `ssd1322` | Własny protokół panelu i adapter SPI | Zachować; GPIO/SPI/DMA SDK osobno |
| `stations` | Model, identyfikatory i CSV | Zachować; własny kod |
| `streaming` | HTTP reader, ICY, M3U/PLS/HLS, live pump | Zachować własne protokoły; klient/TLS SDK osobno, TS vendor wymienić dla B |
| `text` | UTF8, renderer fontów, tabele glyphów | Renderer zachować; bitmapy5x8 wyjaśnić/wymienić, UI16 pozostawić OFL lub zmienić przy C |
| `ui` | HOME, lista stacji, AP, logo, sterowanie | Zachować kod/layout; prawa do danych fontów osobno |
| `ui_widgets` | Widżety, bargraph, wskazówki, volume bar | Zachować; własny kod |


`main` zawiera też warunkowe próby tone/codec/display; nie są to nowe
zewnętrzne biblioteki. Klipy audio są wygenerowanymi sygnałami, a obrazy
w tests/fixtures mogą zawierać kształty fontów wymagające aktualizacji.
Narzędzia Python i testy hostowe są oddzielne od licencjonowania generowanych
danych. FFmpeg/ffprobe uruchamiamy jako procesy, nie linkujemy ich do ESP32.

## 7. Kolejność prac i kryteria gotowości

Dla celu A najkrótsza droga to rozstrzygnięcie małego fontu, ustalenie praw
do własnych plików, dokument MIT obejmujący własny kod z wyjątkami, pełne
noty/source CA i aktualny inwentarz wybranego obrazu. Bibliotek Apache/BSD
nie trzeba wymieniać. OFL większego fontu może pozostać.

Dla celu B, jeśli użytkownik wybierze także pozbycie się ograniczenia
kodeków do Espressif, proponowana kolejność:

1. Pozostawić działający firmware i wprowadzać backendy wybieralne zamiast
   wymieniać cały tor równocześnie. Interfejs source/decode/PCM i ISR zachować.
2. Zachować dr_flac, zrobić PoC dr_mp3 i stb_vorbis z hostowym oracle.
3. Rozstrzygnąć AAC-LC/HE-AACv1/v2 przed obietnicą pełnej migracji:
   licencja, port, peak internal/PSRAM, deadline, SBR/PS i fragmentacja wejścia.
4. Zintegrować libopus z własnym/odrębnie licencjonowanym Ogg i demux TS dla
   HLS. Odróżnić uprawnienia biblioteki kodeka od parsera kontenera.
5. Usunąć obydwa archiwa esp_audio_codec i wszystkie wywołania ich API,
   manifesty oraz opcjonalny wariant vendor. Dopiero wtedy uznać tę
   konkretną restrykcję za usuniętą; potwierdzić mapą/nm, nie samym menuconfig.
6. Przetestować strumienie rzeczywistych stacji, 44.1/48kHz, mono/stereo,
   Ogg chaining, EOF/WAIT/resync, STOP/zmiany stacji, TLS/HLS, CPU i minima
   internal/PSRAM. Nie przenosić alokacji/FFT/I/O do ISR.

Cel C traktować jako odrębny projekt platformy. Przykładowo wymiana tylko
ESP-DSP i kodeków zostawia HTTP/TLS/NVS/sterowniki/radio/libc na innych
warunkach. Nie przedstawiać takiego wyniku jako „cała binarka MIT”.

Ocena wysiłku jest jakościowa: dr_flac już gotowy, MP3/Vorbis umiarkowany
port, AAC/HE-AAC/Opus/TS duży zakres, pełny SDK tylko MIT bardzo duży i bez
zweryfikowanego kompletnego rozwiązania. Nie podano terminów ani obietnic
szybkości bez PoC. Rekomendacja inżynierska: cel A; cel B tylko jeśli
ograniczenie vendorowe przeszkadza w rzeczywistym sposobie dystrybucji.

## 8. Weryfikacja tej analizy

Spis28 własnych modułów porównany z rzeczywistymi katalogami CMake.
103 archiwa CSV porównane z niezerowymi mapowanymi sekcjami bieżącego ELF;
pełne114 rekordów uwzględniają też źródła opcjonalne, zasoby i host tools.
Rodziny licencji oparto na lokalnych notach/manifestach i źródłach
pierwotnych podlinkowanych przy poszczególnych ustaleniach. Kontrola diff
oraz pokrycia/formatu CSV PASS. Nie uruchamiano nowego firmware/builda:
zmiana dotyczy dokumentacji, a działające radio jest testowane przez użytkownika.
Nie nadano nowej licencji i nie dokonano migracji bibliotek w tej iteracji.
