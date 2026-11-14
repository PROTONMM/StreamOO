# SIMD w dekoderach i szybkość FFT — audyt 2026-11-01

## Uzupełnienie ADR0078 — alternatywne FLAC

micro-flac0.3.0/f9c27d4: build S3 zawiera restore_lpc_32bit_asm i
restore_lpc_64bit_asm wywoływane z LPC. Kod źródłowy używa skalarnych mull,
mulsh i pętli sprzętowych; pierwsza pętla32-bit w ELF potwierdza mull/loop.
Nie jest to wektoryzacja PIE. Tablice skoków po wymuszeniu disassembly
potrafią wyglądać jak ee.vmulas — nie zaliczać ich jako instrukcji kodeka.

dr_flac0.13.4/35b5c39 ma backendy SIMD dla SSE/NEON, nie Xtensa. W nowym
adapterze DR_FLAC_NO_SIMD jest jawne, build S3 używa skalarnego C; inspekcja
obiektu nie potwierdza arytmetyki PIE. Własne źródła pozwalają rozważyć
optymalizację, ale żadnej nowej wektoryzacji kodeka nie zaimplementowano.
Oba buildy PASS; porównanie CPU, najdłuższego decode i stabilności na
sprzęcie nadal OPEN. FFT wizualizacji pozostaje ESP-DSP SIMD.

Piny źródeł/licencji: components/decoder/third_party/ORIGINS.json.
Protokół: [FLAC_BACKENDS_BRINGUP](FLAC_BACKENDS_BRINGUP.md).

## Zakres i metoda

Analiza dotyczy konkretnego firmware StreamOO na ESP32-S3, nie wszystkich
implementacji tych formatów. Kodeki pochodzą z przypiętego
`espressif/esp_audio_codec 2.6.2`, a FFT z `espressif/esp-dsp 1.8.2`.
Sprawdzono adapter decoder, symbole, referencje w mapie linkera i disassembly
funkcji w końcowym ELF. Biblioteki kodeków są prekompilowane; kompilator
StreamOO nie wektoryzuje ich ponownie podczas budowania aplikacji.

Artefakt audytu: `build/simd-check/streamoo.elf` z commita `da70514`,
SHA-256 `c63cf7ad0d2daa0fac3f1c5cc977b01f04823dae6f0a2eb8ad59a0e625a01148`.
Archiwum `lib/esp32s3/libesp_audio_codec.a`, SHA-256
`dc511e7ac323fd7a32a50ecb022608ec21a2e4eab3b5e075cf7fa1478612293c`.
Po zmianie wersji biblioteki/targetu audyt trzeba powtórzyć.

SIMD oznacza tutaj arytmetykę na wielu elementach w rejestrach wektorowych
PIE ESP32-S3, np. `ee.vmulas.s16.accx` albo `ee.cmul.s16`.
Skalarne `mulsh`, `mul16s`, pętle sprzętowe, asembler w IRAM i FPU `madd.s`
są optymalizacjami, ale same nie dowodzą SIMD. Również rozszerzone
load/store `ee.ldf.*` nie dowodzą wektorowej arytmetyki.

Nie wystarczy przeszukać całego archiwum lub policzyć `ee.*` w ELF:
nieużywane funkcje enkoderów mogą występować w archiwum, a disassembler
potrafi odczytać tablice danych wewnątrz sekcji kodu jako pozorne instrukcje.
Dlatego pozytywny wynik wymaga zlinkowanej funkcji, prawidłowej pętli kodu
i referencji ze ścieżki dekodera. Wynik negatywny poniżej oznacza brak
potwierdzonej ścieżki SIMD w sprawdzonych gorących funkcjach, nie formalny
dowód o każdym bajcie zamkniętej biblioteki ani o każdym wariancie kodeka.

## Które dekodery korzystają

| Format | Arytmetyka SIMD w sprawdzonym dekoderze | Dowód / ograniczenie |
|---|---|---|
| MP3 | Nie wykryto | Ścieżka `pvmp3_framedecoder`, IMDCT oraz `dct_16_opt` i `pvmp3_polyphase_filter_window_opt`; zoptymalizowany asembler wykonuje skalarne mnożenia `mulsh`. Sufiks `opt` nie oznacza SIMD. |
| AAC-LC | Nie wykryto | `PVMP4AudioDecodeFrame` i stałoprzecinkowe IMDCT/transformacje dekodera. Wektorowy MDCT obecny w archiwum należy do enkodera AAC, nie do tej ścieżki. |
| HE-AAC/AAC+, HE-AACv2 | Nie wykryto | Ten sam dekoder AAC z SBR/PS; zlinkowane `calc_sbr_synfilterbank_opt`, `calc_sbr_anafilterbank_LC_core_opt`, `synthesis_sub_band_LC_core1_opt` i `core2_opt` mają skalarny asembler DSP. Samo włączenie `aac_plus_enable` nie wybiera SIMD. |
| FLAC | Nie wykryto | Odtwarzanie predykcji w `FLAC__fixed_restore_signal`, `FLAC__lpc_restore_signal` i wariantach wide jest skalarne. Nazwy obiektów AVX/SSE/NEON w archiwum nie oznaczają akceleracji S3. |
| Ogg Vorbis | Nie wykryto | Sprawdzono syntezę `vorbis_dsp_synthesis` i transformację `mdct_backward`; brak potwierdzonej ścieżki PIE SIMD. |
| Ogg Opus | Tak, częściowo | `silk_LPC_analysis_filter` wywołuje `opus_silk_lpc_analysis_core`, którego rzeczywista pętla zawiera wektorowe `ee.vmulas.s16.accx`. Wywołania pochodzą zarówno z `silk_decode_core`, jak i PLC. Nie oznacza to wektoryzacji całego CELT/MDCT/Opus. |

Opus: nazwa „analysis” funkcji LPC nie oznacza, że jest to wyłącznie kod
enkodera. Mapa potwierdza jej użycie przez dekoder SILK, również przy
rekonstrukcji po utracie pakietów (PLC). Udział tej funkcji w czasie CPU
zależy od trybu Opus i przebiegu dekodowania; nie zmierzono go osobno.

W archiwum są wektorowe `MDCT_RADIX4FIRST_128ASM`,
`MDCT_RADIX8FIRST_128ASM`, `PRE_LONG_WINDOW_MDCT_128ASM`,
`POST_LONG_WINDOW_MDCT_128ASM`, `LOAD_MDCT_DATA_128ASM` i `Radix4FFT_ASM`.
Referencje prowadzą do `aac_enc_mdct.c.obj`; wymienionych symboli nie ma
w końcowym ELF. Nie zaliczamy ich do akceleracji dekodera AAC.

HLS, MPEG-TS i Ogg są warstwami transportu/kontenerów; korzystają z właściwego
dekodera i nie tworzą osobnego „kodeka SIMD”. Zmiana FFT wizualizacji nie
zmieniła implementacji MP3/AAC/FLAC/Vorbis/Opus.

## FFT wizualizacji

Od `6f27c8b` flaga `CONFIG_STREAMOO_DSP_ENABLE_SIMD` rzeczywiście wybiera
`dsps_fft2r_sc16_aes3_`. Końcowy ELF zawiera bezpośrednie wywołanie z
`streamoo_spectrum_analyze` i wektorowe zespolone mnożenia oraz butterfly:
`ee.cmul.s16`, `ee.vmul.s16`, `ee.vadds.s16`, `ee.vsubs.s16`.
Hann/DC, skalowanie blokowe, bit reversal i agregacja pasm pozostają skalarne.
Po wyłączeniu flagi wraca własny float radix2. Szczegóły: [ADR0074](adr/0074-vector-sc16-fft-and-measurement.md).

### Czy przyspieszyło i o ile

Logi wskazują na krótszy typowy przebieg, ale nie zawierają kontrolowanego
pomiaru A/B pozwalającego przypisać dokładny współczynnik samej wektoryzacji.
Obecne `last_fft`, `avg_fft`, `max_fft` mierzą cały feed z analizą stereo,
łącznie z wywłaszczeniami. Nie są benchmarkiem samego kernela FFT.

| Pomiar | Dostępny czas | Jak go interpretować |
|---|---|---|
| Float, firmware `53e7e2b`, log przy56.046s | `frames=1`, `max_fft=4104us` | Pierwsze pełne widmo stereo; 4.104ms jest czasem tej próbki, nie średnią serii. |
| SIMD, firmware `6f27c8b`, HLS gen4, typowe widoczne próbki | np. `last_fft=2589`, `2723`, `2278us` | Około2.3–2.7ms dla tych próbek; inne próbki trwają około4ms lub dłużej. |
| SIMD, ten sam HLS przy131.367s | `avg_fft=7402us`, `max_fft=513616us` | Średnia zawiera okresy odsunięcia zadania; maksimum513.6ms nie oznacza, że tyle oblicza kernel. |
| SIMD, FLAC gen6 przy221.577s | `avg_fft=3630us`, `max_fft=23130us` | Średnio3.63ms całej analizy w tej sesji, przy innym obciążeniu niż HLS. |

Iloraz wybranej starej próbki4.104ms i nowych2.3–2.7ms wynosi około
**1.5–1.8×**, czyli czas krótszy o około34–44%. Przykład4.104/2.5 daje
1.64× i39% mniej czasu. To wyłącznie orientacyjne zestawienie pojedynczych
próbek z różnych przebiegów, a nie potwierdzony speedup ani pomiar średnich.
Nie można na jego podstawie obiecać stałego przyspieszenia każdej klatki.

Starsze duże wartości `max_fft` (dziesiątki/setki ms) także zawierają
wywłaszczenia; porównywanie ich z nową typową próbką i ogłaszanie
kilkunasto-/kilkusetkrotnego przyspieszenia byłoby błędne. Nie ma też starego
`avg_fft`, więc nie podajemy procentowej poprawy średniego czasu.

Wniosek: arytmetyka wektorowa jest potwierdzona, poprawa typowego czasu jest
sugerowana przez próbki, a dokładna poprawa pozostaje **OPEN** do A/B.
Cała analiza nadal zużywa CPU na skalarne etapy i bywa wywłaszczana.
Dlatego po SIMD pozostały luki widma, mimo zapasu CPU; zmianę priorytetu
i rdzenia z `da70514` trzeba ocenić osobno od zmiany backendu.

### Co jest potrzebne do miarodajnego porównania

Ten sam kod `da70514` lub nowszy, identyczna stacja/PCM, N1024 i widget,
ten sam core/priority, dwa buildy różniące się tylko flagą StreamOO SIMD.
Po rozgrzaniu zebrać co najmniej60s danych. Najlepiej użyć powtarzalnego PCM
i zmierzyć CPU/cykle samego kernela oraz czas całej analizy oddzielnie.
Oddzielić koszt zmian float→sc16, skalowania i wektoryzacji, jeśli celem jest
wyłącznie udział SIMD. Procedura użytkowa: [DSP_BRINGUP](DSP_BRINGUP.md).

## Pochodzenie danych i odtworzenie audytu

Log float: załącznik `bfa4fc39-dc67-40c5-97ae-70e8dc2ae25a`, SHA-256
`20a16880ab50170a9cb1b5005e3be575b1b6c4c89f0519b7cf92c05cb383005c`.
Log SIMD: załącznik `c27a8350-461e-443b-8fc3-cdfe6e7c0a2d`, SHA-256
`4b4d86d6340b23b535db56973a4eabc57f91315a137ed3ded3c7e813b4a1a830`.
Pełnych logów z danymi sieci użytkownika nie dodajemy do repozytorium.

Przykładowe polecenia z katalogu projektu, po aktywacji toolchainu S3:

```sh
xtensa-esp32s3-elf-nm --defined-only build/simd-check/streamoo.elf
xtensa-esp32s3-elf-objdump -d --disassemble=opus_silk_lpc_analysis_core build/simd-check/streamoo.elf
xtensa-esp32s3-elf-objdump -d --disassemble=pvmp3_polyphase_filter_window_opt build/simd-check/streamoo.elf
xtensa-esp32s3-elf-objdump -d --disassemble=synthesis_sub_band_LC_core1_opt build/simd-check/streamoo.elf
xtensa-esp32s3-elf-objdump -d --disassemble=dsps_fft2r_sc16_aes3_ build/simd-check/streamoo.elf
```

Referencje wywołań sprawdzać w `build/simd-check/streamoo.map`, pamiętając
o odrzuconych sekcjach i obiektach o tej samej nazwie w archiwum.
To audyt statyczny i analiza istniejących pomiarów, bez nowego benchmarku
na sprzęcie. Wersje/licencje: [THIRD_PARTY_NOTICES](../THIRD_PARTY_NOTICES.md).
