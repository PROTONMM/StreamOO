# ADR0037 — niezależny rdzeń pomiarów RMS/peak stereo

Data: 2026-10-31. Status: przyjęte; etap07a, integracja DSP jeszcze OPEN.

## Decyzja

Przed uruchomieniem FFT wdrażamy własny komponent levels bez zależności od
UI, sterownika, RTOS i gain. API przyjmuje blok signed32 stereo48kHz przed
regulacją, długości1..240 ramek oraz sequence. Jeden właściciel: przyszłe
zadanie DSP. Nie wywoływać w callbacku I2S. Bez alokacji i I/O.

Domyślne okno1920 ramek/40ms, konfigurowalne1..48000. Okna nienakładające
się. Dzielenie signed32 przez65536 w stronę zera daje Q15 (dokładne dla
signed16 rozszerzonego do32bit). Suma kwadratów uint64 mieści do48000 próbek
na kanał; kwadrat największej wartości32768 nie przepełnia. RMS to całkowity
pierwiastek średniej kwadratów, peak to maksimum modułu, wynik0..32767
z saturacją pełnej ujemnej skali. Zaokrąglenie RMS w dół, błąd<1 jednostki
Q15 względem referencji z tych samych skwantyzowanych próbek.

Luka/duplikat sequence resetuje niepełne okno i poprzedni wynik. Rollover
uint32 jest ciągły. Nowa stacja/STOP wymaga init od właściciela. Odbiorca
widzi ostatnie kompletne okno i licznik ukończonych okien; brak nowego wyniku
nie oznacza ciszy. Niepoprawne parametry nie zmieniają kontekstu.

## Granice i weryfikacja

Nie dodano zależności: C11/stdint/stdbool/string z istniejącego toolchainu
ESP-IDF6.1; własne API i implementacja, brak importu kodu stron trzecich.
Nowy komponent kompilowany przez main, jeszcze niewywoływany w firmware.
Brak zmian transportu PCM/tap, ISR, playera i układu OLED.

Dziewięć testów C: niezależna referencja float dla sinusów100/1000/8000Hz,
stereo, cisza, impuls, INT32_MIN/MAX z maksymalnym oknem, granice bloków,
przepełnienie numeru sequence, luki/duplikaty, reset, kwantyzacja i błędne API.
Test rzeczywistego pcm_fill/tap/gain potwierdza te same RMS/peak przy
volume0/50/100, przy faktycznie wyciszonym wyjściu dla0. ASan/UBSan PASS
(LSan wyłączony). To test obliczeń/kontraktu, nie deadline na S3.

Kolejny mały krok: oddzielny konsument droppable tap, publikacja spójnego
snapshotu z generacją/czasem do istniejących bargraph/analog, bez starych
poziomów po STOP/stacji. Dalej smoothing, kalibracja dB/VU i FFT z Hann N−1,
sprawdzeniem ESP-DSP API/licencji/wersji przed dodaniem zależności.
