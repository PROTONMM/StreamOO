# ADR0074 — wektorowy FFT sc16 i pomiar na S3

Data: 2026-11-01. Status: build/testy PASS, odbiór i pomiary S3 OPEN.

Użytkownik chce FFT wektorowy niezależnie od wyniku profilowania CPU.
Wybieramy oficjalny ESP-DSP1.8.2, Apache2.0, przypięty w manifeście i locku.
Sprawdzono API, licencję, źródła i warunek IDF>=4.2; native build IDF6.1
potwierdza kompatybilność. Zależność jest prywatna dla spectrum.

## Backend i własność

`CONFIG_STREAMOO_DSP_ENABLE_SIMD=y` rzeczywiście wybiera teraz
`dsps_fft2r_sc16_aes3_`, potem `dsps_bit_rev_sc16_ansi`. Jawny wybór AES3
nie pozwala przypadkowo przestawić transformacji na ANSI przez flagę
optymalizacji biblioteki. Oba wejścia/wyjścia są complex interleaved int16;
wykonujemy osobne FFT L/R i łączymy moc jak wcześniej. Float AES3 używa
skalarnego FPU z rozszerzonymi load/store, dlatego nie spełnia celu SIMD
arytmetycznego w tym kroku. Wyłączenie flagi StreamOO przywraca własny
backend float do kontrolowanego porównania A/B.

Kontekst w PSRAM i pole robocze mają wyrównanie16B. Jedna statyczna tabela
sc16 dla N<=2048 (4KiB internal RAM) żyje przez cały firmware, bez alokacji
wewnątrz init ESP-DSP. Jeden task metering inicjuje i używa backendu;
zakończenie sesji czeka na ACK przed otwarciem następnej. Inny właściciel
globalnej tabeli ESP-DSP jest wykrywany jako błąd. Nie wolno deinitować
tej tabeli z innego komponentu. Błąd init/alokacji wyłącza FFT, zachowuje RMS
i audio. Nie ma alokacji, init tablic, I/O ani blokad wewnątrz analyze.

Hann N-1, ważone usuwanie DC, pasma, stereo, gamma, takt10Hz, overlap50%,
task affinity i ścieżka I2S pozostają takie jak wcześniej. Rdzeń nadal
obsługuje1024/2048, kolektor live ma1024. Okno każdego kanału normalizujemy
do peak16383, zostawiając6dB headroom; po FFT odwracamy tę skalę i DFT/N.
To skalowanie numeryczne, nie AGC prezentacji: wynik zachowuje poziom dBFS.
Chroni też sygnały ciche i impulsy/DC przed obcięciem przy konwersji sc16.
Przygotowanie okna, bit reverse i agregacja pasm pozostają skalarne.

## Pomiary i weryfikacja

Log FFT podaje nazwę backendu oraz `last_fft`, `avg_fft`, `max_fft` w us.
Są to czasy ścienne całego feed produkującego widmo stereo (z wywłaszczeniem),
a nie sam kernel ani procent CPU; średnia i maksimum dotyczą sesji.
Profiler ADR0073 nadal mierzy runtime tasków i idle rdzeni co10s.
Procedura A/B jest w DSP_BRINGUP. Bez pomiaru S3 nie deklarujemy speedupu,
zniknięcia luk ani zmiany obciążenia CPU.

Native build IDF6.1/GCC15.2.0 PASS, obraz0x17e240 (90% factory wolne).
ELF zawiera wywoływany `dsps_fft2r_sc16_aes3_` z `ee.vld.128`,
`ee.cmul.s16`, `ee.vmul.s16`, `ee.vadds.s16`; tabela sc16 ma4096B i16B alignment.
Build wykonano w `build/simd-check` z odizolowanym venv w `/tmp`, ponieważ
standardowy lokalny venv6.1 jest nieobecny. Pakiety spełniają lokalny plik
requirements.core, bez brakującego globalnego constraints.v6.1.

Test hostowy kompiluje niezmieniony ANSI sc16 ESP-DSP i porównuje adapter
z własnym float oraz NumPy: N1024/2048, trzy sample rates, cisza/DC,
Nyquist, fazy, pojedynczy kanał, off-bin, szum, impuls i poziomy od0.0001FS.
Tolerancja amplitudy0.0025FS/82Q15, błąd tonu coherent<0.5%; nie obiecujemy
bit-identycznego wyniku float. Sprawdzono mniejszy workspace, canaries,
niezmienność wejścia, wyrównanie i kolizję globalnej tabeli.
Host ANSI oracle i ASan/UBSan nie zastępują wykonania AES3 na S3.
392 testy hostowe PASS, osiem testów sc16 wraz z native harness ASan/UBSan
PASS (LSan off); oba walidatory konfiguracji oraz diff --check PASS.
Brak flash/erase/eFuse; NVS, partycje i prywatne playlisty nietknięte.

## Uzupełnienie pomiarów — 2026-11-01

Log6f27c8b potwierdził wykonanie backendu na S3. Szczegółowy audyt kodeków,
wartości z logów float/SIMD i ograniczenia porównania są w
[CODEC_SIMD_AUDIT](../CODEC_SIMD_AUDIT.md). Pojedyncza próbka float4.104ms
wobec wybranych SIMD2.3–2.7ms sugeruje około1.5–1.8×, ale to orientacyjny
iloraz różnych przebiegów, nie benchmark A/B ani pomiar zysku samego SIMD.
Średnia i najgorszy czas zależą od wywłaszczeń; poprawkę scheduling ADR0075
należy oceniać osobno. Dokładny speedup i soak nadal OPEN.
