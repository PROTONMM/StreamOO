# ADR0041 — referencyjny rdzeń widma FFT

Data: 2026-10-31. Status: przyjęte, etap07d. Integracja live/ESP-DSP OPEN.

Użytkownik zostawia RMS/peak w obecnej postaci do późniejszego strojenia.
Nie zmieniamy ich parametrów ani UI. Dodajemy własny komponent spectrum:
przenośny backend referencyjny float radix2, N1024/2048, jako punkt
odniesienia dla późniejszej wersji zoptymalizowanej. Nie uruchamiamy
niezmierzonego backendu podczas odtwarzania.

## Kontrakt matematyczny

Wejście signed32 stereo przed gain, jawny sample_rate8..48kHz. Workspace
od właściciela, około37KiB, bez alokacji/globalnego stanu/I/O. Init liczy
Hann symetryczny0.5*(1-cos(2*pi*n/(N-1))) i twiddles raz. Mean każdego
kanału odejmowane przed oknem, DC wyniku0. FFT w miejscu, następnie
normalizacja amplitudy przez sumę okna; dodatnie biny razy2, Nyquist razy1.
Stereo: średnia mocy L/R, potem pierwiastek; przeciwne fazy nie znoszą
się, pojedynczy aktywny kanał daje amplitudę/sqrt2 względem dwóch.

24 rozłączne, przybliżone logarytmiczne pasma pokrywają wszystkie biny
1..N/2, bez dziur i bez pustych pasm. Dolne granice ograniczone rozdzielczością
FFT (48kHz:46.875Hz dla1024 lub23.4375Hz dla2048), nie obiecujemy20Hz
rozdzielczości dla1024. Agregacja: maksymalna amplituda binu w paśmie,
Q15 liniowe z saturacją32767. To amplituda tonu, nie RMS/pasmowa suma mocy
ani docelowe skalowanie dB. Nie dobieramy pasm na stałe do nazw częstotliwości.

## Wybór przyszłej biblioteki

Sprawdzono oficjalny ESP-DSP v1.8.2, Apache2, manifest IDF>=4.2,
API dsps_fft2r_init_fc32/dsps_fft2r_fc32/dsps_bit_rev_fc32 oraz warianty
sc16/aes3. Init może alokować przy NULL, tablice globalne i kolejność
bit-reverse wymagają świadomego adaptera/własności; nie kopiujemy wywołań
na ślepo do I2S. Manifest dopuszcza IDF6.1, lecz nie dowodzi wydajności
ani kompatybilności wybranej konfiguracji — potrzebny build i pomiar S3.

Źródła audytu (2026-10-31):
- https://github.com/espressif/esp-dsp/tree/v1.8.2
- https://raw.githubusercontent.com/espressif/esp-dsp/v1.8.2/idf_component.yml
- https://raw.githubusercontent.com/espressif/esp-dsp/v1.8.2/modules/fft/include/dsps_fft2r.h

Nie dodano teraz ESP-DSP do zależności i nie importowano kodu stron trzecich.
Własny referencyjny FFT pozwala najpierw utrwalić normalizację i testy,
a następnie wymienić backend z porównaniem błędu/czasu float vs sc16.
NumPy1.26.4 użyte wyłącznie jako zewnętrzny oracle testowy, nie w firmware.
Dwa testy oracle są opcjonalne przy braku NumPy; ich skip nie oznacza PASS
porównania. Pozostałe siedem testów nie wymaga NumPy.

## Sprawdzenie i granice

Dziewięć testów: Hann N-1/symetria/suma/kwantyzacja Q15 współczynników,
sinusy100/1000/8000Hz i rate8/44.1/48kHz z NumPy rFFT, N1024/2048,
pełnoskalowy szum/impuls/cisza/DC/Nyquist, fazy stereo i pojedynczy kanał,
pokrycie wszystkich binów, poprawność normalizacji i brak zapisu wejścia.
Native C harness sprawdza canaries oraz rzeczywisty PCM/tap przy volume0/
50/100: widmo identyczne, wyjście dla0 wyciszone. Ten harness ASan/UBSan
PASS (LSan off); NumPy/ctypes testy bez instrumentacji ASan.

Brak jeszcze kolektora okien z50% overlap, resetu po lukach, adaptera
ESP-DSP/SIMD, FFT sc16, pomiarów CPU i publikacji do OLED. Zwykły build
kompiluje komponent; niewywoływany kod usuwa linker, ekran bez zmian.
Nie trzeba wgrywać firmware tego kroku. Następnie adapter backendu,
kolektor i live snapshot liczone tylko dla wybranego widżetu FFT.

Weryfikacja07d:295 testów hostowych PASS (oracle NumPy wykonany, bez skip).
Native C FFT/PCM harness ASan/UBSan PASS, LSan off. Native ESP-IDF6.1 build
PASS, obraz0x15bf20 (7% wolne); spectrum kompilowany, nieużywany kod
usunięty przez linker. check_config --platform i diff --check PASS.
Bez flash/erase/eFuse i zmian partycji.
