# ADR0071 — kontrast prezentacji widma OLED

Data: 2026-10-31. Status: zaimplementowane, odbiór OLED OPEN.

## Problem

Użytkownik ocenił widmo jako zbyt masywne i chciał, aby piki oraz głośniejsze
częstotliwości były lepiej widoczne. Weryfikacja wykazała, że 24 zakresy
częstotliwości są już logarytmiczne. Amplituda jest przeliczana na -60..0dBFS,
ale wynik był dalej liniowo rozciągany na wysokość słupka, a cały słupek miał
jednolitą jasność12.

## Decyzja

Po istniejącym przeliczeniu dB normalizujemy `level` krzywą gamma1,5:
`level * sqrt(level)`. Zmniejsza to wysokość cichego tła bez zmiany progu
ciszy, pełnej skali lub częstotliwościowych granic pasm. Przykładowo sygnał
-20dBFS ma około54% wysokości zamiast67%.

Słupek ma korpus o jasności8 i jednoliniowy jasny szczyt15. Ta linia pokazuje
bieżącą wysokość po istniejącym envelope; nie jest oddzielnym peak-holdem.

## Granice

Zmiana dotyczy wyłącznie zadania meteringu i renderingu gotowego snapshotu.
Nie zmienia FFT, okna Hann, tempa10Hz, envelope320ms, gain, PCM, I2S, NVS,
flash ani sieci. `sqrtf` jest poza ścieżką próbek I2S i jest wywoływane
maksymalnie dla24 pasm nowej klatki FFT.

381 testów hostowych PASS; testy sprawdzają monotoniczność skali, punkt
-20dBFS oraz jasny szczyt i przyciemniony korpus. Walidator GPIO PASS.
Natywny build ESP-IDF6.1 jest w tej sesji niedostępny, ponieważ lokalny
`export.sh` wskazuje brakujący virtualenv `idf6.1_py3.12_env`. Potrzebny
jest odbiór na OLED przy mowie i muzyce; dalsze strojenie gamma lub osobny
peak-hold jest możliwe później.
