# Aktualny odbiór07k — terminowość RMS/FFT (ADR0075)

Po zwykłym build/flash, bez erase, testuj HLS64 HE-AACv2 przez60s z FFT,
30s bargraph i ponownie FFT, następnie HE-AAC bez HLS oraz FLAC.
Log startowy ma teraz potwierdzić `RMS/FFT core=0 priority=4`.
Profiler ma pokazywać audio_levels affinity0x1, a meters core_samples=N/0.
Porównuj przyrosty drop/gaps oraz last/avg/max_fft i ciągłość frames,
zachowując low/hard/starts audio. Sprawdź też kilka zmian stacji: procenty
nowych readerów nie mogą przepełniać się wskutek ponownego użycia TCB.

Pomiar6f27c8b: HLS FFT zwykle2–4ms, ale max wall513.6ms przy PCM32,
low0/hard0/starts1. Core0 około36%, core1 około59%; analiza około7.5% CPU.
Wcześniej task po pierwszym FPU przypinał się przypadkowo do0 albo1,
priority1 pozwalał reader3 lub dekoderowi4 odsunąć analizę. Nowy core0/
priority4 ma ograniczyć wywłaszczenia przez reader; skuteczność OPEN na S3.
Nie zmieniono okna/algorytmu SIMD, bufora PCM, expiry ani prezentacji.

---

# Odbiór07j — FFT SIMD i CPU (ADR0074)

1. Jeżeli na płytce jest jeszcze53e7e2b, zbierz punkt odniesienia przed
   nowym flashem: działający HLS HE-AACv2, FFT na HOME,60s logu. W logu
   musi być PLAYING/phase4, rosnące frames i prawidłowe PCM; błąd TLS nie
   mierzy dekodowania. Pierwszy blok CPU jest dopiero około20–22s po boot.
2. Zbuduj bieżący firmware i wgraj zwyczajnie bez erase/NVS/obrazu FS.
   Domyślna flaga SIMD jest włączona; log FFT ma wskazać
   `backend=esp-dsp-sc16-aes3-simd`. Agent zweryfikował build w osobnym
   `build/simd-check`; dla tego katalogu użyj `idf.py -B build/simd-check`
   zamiast domyślnego `idf.py`. Standardowy build użytkownika też jest poprawny.
3. TEST HLS64k HE-AACv2:60s FFT, potem30s bargraph, ponownie30s FFT.
   Powtórz dla działającego TEST FLAC native lossless. Sprawdź też VOL0,
   STOP/PLAY, zmianę stacji oraz powrót z listy; widmo ma reagować, audio
   grać czysto. Przyciski/wygaszanie pozostają zgodne z ustawieniami.
4. Zbierz bloki `CPU interval`, `CPU task`, `player phase`, `Decode perf`,
   `meters`, `FFT`, heap. Porównaj `audio_levels`/`player` load i affinity,
   `core_samples`, `last_fft/avg_fft/max_fft`, frames/gaps/drop oraz low/hard/starts.
   Wartości max/średnie FFT są od początku sesji i zawierają wywłaszczenia.
   `core_samples` liczy iteracje, nie udział procentowy czasu rdzeni.
5. Jeśli brak starego pomiaru, A/B można zrobić na tym samym kodzie:
   w menuconfig Audio and DSP wyłączyć flagę StreamOO SIMD, zbudować/wgrać,
   zebrać log `portable-float-radix2`, potem ponownie włączyć i powtórzyć.
   Porównywać tę samą stację/rate/widget i podobny fragment po rozgrzaniu.

OPEN: czas i poprawność wykonania AES3 na S3, wpływ na luki wizualizacji
i rzeczywiste CPU. Host/NumPy testuje skalowanie przez ANSI sc16, a ELF
potwierdza instrukcje SIMD; żaden z tych testów nie mierzy czasu na płytce.

---

Historyczny krok HOME/ADR0044: zwykły flash bez erase/NVS. Po wybraniu stacji
w dolnym lewym panelu oczekuj MP3 lub AAC oraz RSSI (dBm) i słupków Wi-Fi.
STOP czyści kodek do placeholdera; następny PLAY odtwarza informację po
dekodowaniu. Bitrate nadal --. RSSI odświeżane co1s, nie steruje roamingiem.
Sprawdź MP3/AAC na znanych źródłach; wygląd FFT/RMS pozostawiony bez zmian.

Aktualizacja07f (ADR0043): poprawione DC ważone Hann, FFT z natychmiastowym
atakiem i opadaniem pełnej skali320ms. Sprawdź mowę/muzykę: krótka cicha
klatka nie powinna zerować wysokich słupków natychmiast. Pełna cisza daje
pusty wykres z linią; brak świeżych pomiarów daje „Brak danych”.
STOP/zmiana stacji nadal czyści od razu. RMS/peak bez zmian.

# Aktualny odbiór07e — widmo FFT z audio

1. Zwykły flash, bez erase/NVS/obrazu FS. Wybierz działającą stację,
   wciśnięciem i obrotem wybierz FFT. Oczekuj24 reagujących słupków.
   Skala -60..0dBFS, aktualizacja maksymalnie10Hz, okno1024 przy48kHz.
2. VOL0 nadal pokazuje widmo; próbki są przed gain. Zmiana na bargraph/
   analog lub wejście na listę wyłącza obliczenia FFT. Licznik frames
   powinien wtedy stać; po powrocie czekamy na świeże pełne okno.
3. STOP/PLAY i szybkie zmiany stacji: stare słupki nie mogą wracać,
   buffering/brak sieci usuwa widmo. Brak pomiarów przez200ms daje placeholder.
4. Słuchaj pod kątem płynności. Prześlij15–30s logów player/platform z FFT
   i potem innym widżetem: max_fill/invalid/low/hard, meters drop/gaps/
   max_work/stack_hwm, FFT available/valid/frames/skipped/gaps/max_fft,
   heap internal free/min/largest i PSRAM. Bez prywatnych URL/haseł.

Skipped jest oczekiwany:50% overlap daje więcej okien niż takt10Hz; większość
transformacji pomijamy. Gaps/drops oznaczają utracone próbki analizy, nie
automatycznie awarię audio. Max_fft to czas ścienny feed z FFT, nie pomiar
procentowego CPU. Pierwszy init może podnieść max_work.

To pierwszy odbiór live backendu reference, jeszcze bez deklaracji SIMD/
wydajności S3. Dodatkowe około45KiB PSRAM, brak nowego zadania/dużego
bufora internal. Brak PSRAM oznacza FFT available=0, audio/RMS mają działać.
Próba alokacji ponownie po wyłączeniu i włączeniu FFT.

---

# Odbiór07b — rzeczywiste RMS/peak na OLED

1. Wgraj zwykłym `idf.py -p PORT flash`, zachowaj NVS/LittleFS, bez erase
   i obrazu FS. Wybierz znaną działającą stację.
2. Wciśnij enkoder i obracaj, wybierz bargraph L/R albo dwa mierniki
   analogowe. Powinny reagować na audio. W07e FFT ma już rzeczywiste pasma.
3. Zmień volume na0,50,100. Wskaźniki mają nadal reagować przy mute, bo
   pomiary są przed gain. Dla zmiennej audycji wartości naturalnie się
   zmieniają — porównanie liczb wymaga tego samego fragmentu/sygnału.
   Test stałych próbek przez rzeczywisty pcm_fill/tap/gain już PASS na hoście.
4. STOP ma usunąć poziomy (placeholder), PLAY przywraca je po nowym oknie
   i wyjściu z buffering. Zmień stację, także szybko dwa razy: poziomy starej
   stacji nie mogą pozostać. Odtwarzanie podczas przeglądania listy zostaje.
5. Sprawdź płynność i brak trzasków. Zbierz około15–30s logu player/platform
   przy odtwarzaniu i po kilku zmianach, bez prywatnych URL/credentials.
   Istotne: max_fill/invalid/low/hard, heap internal free/min/largest,
   `meters valid/windows/gaps/drop/max_work/stack_hwm/rms/peak`.

Max_work to czas ścienny iteracji konsumenta, również możliwe wywłaszczenia,
nie pomiar procentowego CPU. Spodziewane normalnie gaps/drop0; sporadyczne
odrzucenie pomiarów nie powinno powodować przerwy audio. Test sztucznego
pełnego bufora na hoście potwierdza drop-newest i reset okna po luce; nie
zastępuje pomiaru wydajności na urządzeniu. Długotrwałe przeciążenie powoduje
„Brak danych”, wynik starszy niż200ms jest unieważniany.

Błąd `Audio meters disabled` oznacza brak pamięci/zadania; audio ma grać
dalej. Sprawdź wtedy heap. Około10KiB kontekstu i4096B stosu plus TCB
tworzone na sesję; po STOP kontekst zwalniany po ACK, stos przez idle task.

07c: okna pomiarowe40ms, prezentacja10Hz, RMS attack30/release150ms,
peak hold150/release300ms. Sprawdź spokojniejszy ruch i brak migania przy
krótkich lukach tap. STOP/zmiana stacji nadal czyści od razu, a brak nowych
kompletnych pomiarów przez200ms daje placeholder. Dalsze strojenie skali
dB/VU oraz FFT/Hann/ESP-DSP pozostają kolejne.
