# ADR0078 — wybieralne backendy FLAC i wspólne pomiary

Data:2026-11-01. Status: host/build PASS; testy odsłuchowe i CPU S3 OPEN.

Użytkownik zlecił test micro-flac i dr_flac oraz możliwość wyboru biblioteki,
porównanie CPU/buforów i sprawdzenie SIMD. Wybór w Kconfig, domyślnie
micro-flac, z zachowaniem Espressif jako wariantu porównawczego. Wymiana
tylko FLAC, bez nowego frameworka audio/Arduino, zmian API WWW i transportu.

## Log3ee4ad16

W gen1/2/3 sesja kończy się stage=decode/vendor-7. Odpowiedni Reader:
ended0/errorESP_OK/HTTP200, read1024 i wzrastające rx. W przesłanym pliku
nie ma wpisu disconnect Wi-Fi ani błędu HTTP/TLS poprzedzającego te awarie.
Potwierdzony błąd dekodowania, nie dowód na wcześniejsze zerwanie sieci;
player po błędzie sam zamyka/restartuje sesję. Wcześniej wiele empty WAIT
i low, mimo okresów pełnego ring. Przyczyna konkretnego vendor-7 OPEN.

Znaleziono niezależny błąd zakresu readera nagłówków: RFC9639§4.1/9.1.6
dopuszcza końcowy blok1..15 próbek. Strukturalny czytnik granic nie może
go odrzucać jak bootstrap STREAMINFO. Poprawka ma oracle końcowych7 próbek
w obu prawdziwych backendach. Nie przypisujemy jej automatycznie awarii logu.
[RFC9639](https://www.rfc-editor.org/rfc/rfc9639.html#section-4.1).

## Zależności i interfejs

Źródła zweryfikowane przed importem, przypięte i lokalnie vendored w
components/decoder/third_party. ORIGINS.json podaje commity i SHA-256 plików.

- micro-flac0.3.0, f9c27d4d0166021db977d7fc37d205b21d81e9f2,
  Apache-2.0, komponent deklaruje IDF>=4.4. Import tylko natywnego dekodera,
  bez enkodera i bez Ogg/submodułu. Włączone oryginalne ASM Xtensa LPC,
  workspace PSRAM-only, CRC włączone. Publiczny interfejs decode z
  consumed/samples/NEED_MORE_DATA, PCM32 MSB-aligned, reset/destruktor.
- dr_flac0.13.4,35b5c392cc15456736c440a68dbe073178bbffe0,
  wybrana MIT-0. Jednoplikowy C bez stdio/Ogg/SIMD. Publiczne open/read_pcm/
  close i allocation callbacks, source callbacks podają tylko gotową ramkę.
  Jeden handle na sesję, nie reopen per ramka. Cache około132KiB PSRAM;
  źródło pull nie widzi WAIT. Własny parser z ADR0077, więc nie jest to
  niezależny test całej warstwy granic FLAC. Test micro-flac jest niezależny
  od tego parsera; wspólny jest tylko bootstrap live.

Żadnych zmian w źródłach obu bibliotek; podzbiór plików i prywatne flagi
CMake opisane w notice. Bez nowego GPL. Licencje, nagłówki i referencje
zachowane. Espressif nadal2.6.2 dla pozostałych kodeków i wariantu FLAC.
Nie wprowadzamy I/O, alokacji ani parsera do ścieżki I2S.

Adapter C dla wspólnego pipeline, prywatny C++ tylko dla micro. Utrzymany
kontrakt consumed/produced, bootstrap i reset. Alternatywy oddają PCM32;
resampler/gain/FIFO/tap bez zmian. Wariant dr nie jest bezpośrednim readerem
sieciowym: dopasowanie API blokuje pomylenie chwilowego braku danych z EOF.
Przy EOS zachowuje ostatni bajt wejścia do opróżnienia gotowych ramek.

## Pomiary i ograniczenia

Wspólne bufory/progi i priorytety. Krótsze linie Decode perf/buffers/flow
zamiast jednej długiej, która w logach użytkownika często była ucięta.
Backend i avg_call w perf; min/max napełnienia źródła i FIFO z obserwacji
ownera w interwale. Statystyki RTOS nadal mierzą CPU, czasy wrappera są
wall time. Pełny protokół: [FLAC_BACKENDS_BRINGUP](../FLAC_BACKENDS_BRINGUP.md).

Audyt: micro LPC ma skalarne instrukcje mull/mulsh; ELF zawiera oba symbole
i ścieżki wywołań. Inspekcja pierwszej pętli32-bit potwierdza mull i loop.
Usunięcie .xt.prop tylko z kopii ELF do disassembly odsłania tablice skoków:
ich bajty potrafią wyglądać jak ee.vmulas. Nie zaliczamy tych pozornych
instrukcji do SIMD. dr object nie ma potwierdzonej arytmetyki PIE, upstream
obsługuje SIMD SSE/NEON, wyłączone w tym adapterze. FFT pozostaje wektorowy.

409 host tests PASS, realne nowe backendy6 i actor11 ASan/UBSan PASS.
FFmpeg oracle i178 ramek poprzedniego capture PASS. Native IDF6.1:
micro0x17b150,dr0x17e750,Espressif0x17ea10;90% factory wolne.
Nie wykonano flash/erase/eFuse ani pomiarów CPU/odsłuchu na fizycznym S3.
