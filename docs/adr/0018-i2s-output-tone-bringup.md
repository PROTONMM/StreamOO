# ADR 0018 — I2S i skończony test PCM5102A

Data: 2026-10-24. Status: przyjęty dla 06a; test DAC na sprzęcie OPEN.

Własny moduł audio_output używa nowego esp_driver_i2s z ESP-IDF 6.1
(Apache-2.0, lokalne nagłówki i źródła sprawdzone). Nie dodano pakietów,
nie kopiowano implementacji SDK. Publiczne i2s_common.h/i2s_std.h oraz
dokument Data Transport w docs/en/api-reference/peripherals/i2s.rst
potwierdzają dostęp do DMA bezpośrednio w callbacku on_sent.

Dokumentacja TI PCM510xA SLAS859C:
https://www.ti.com/lit/ds/symlink/pcm5102a.pdf
Sekcje 9.3.2/9.3.6 i tablica 11 opisują I2S, PLL z BCK oraz SCK na GND
przy pracy bez MCLK. FMT musi wybierać I2S, XSMT nie może wyciszać DAC.
Wybrany profil: ESP master I2S0, Philips stereo L/R, 48kHz, słowo/slot32,
BCK=64fs=3.072MHz, WS=48kHz; MCLK nie wyprowadzamy. Piny 48/47/21
pozostają zgodne ze schematem referencyjnym. To nie potwierdza zasilania,
zworek ani połączeń konkretnego modułu użytkownika; kontrolne piny DAC
nie dostają wymyślonych GPIO. Dokładny model/strapping modułu nadal OPEN.

Transport: 4 deskryptory DMA po 240 ramek (5ms, 1920B), na starcie jawnie
wypełnione zerami przed włączeniem zegarów. on_sent dostaje bufor zakończony,
SDK czyści go przed callbackiem, a nasz callback wypełnia całość na następny
obrót DMA. Nie używamy i2s_channel_write/read w ścieżce próbek. Nie ma
alokacji, flash, blokującego I/O, logów, FFT ani float w callbacku.
Nowy STREAMOO_I2S_SMOKE_TEST domyślnie y wybiera I2S_ISR_IRAM_SAFE
(i zależne GDMA ISR w IRAM). Callbacki/generator są w IRAM; tablica fali,
stan generatora i kontekst transportu w wewnętrznym RAM. Adresy i brak
wywołań helperów spoza IRAM generatora sprawdzono w ELF/disassembly.

Generator aplikacyjny audio_tone, nie część produkcyjnego renderera/audio:
oryginalna tablica 48 próbek sinusa, peak1036 w ekwiwalencie int16,
~-30dBFS peak; słowa int32 są skalowane przez 65536 (bez UB signed shift).
10s: zero1s, L1k2s, zero1s, R2k2s, zero1s, oba1k2s, zero1s.
Rampy liniowe obwiedni 50ms na obu końcach każdego tonu. Generator jest
skończony, po końcu nasyca licznik i emituje zera, niezależnie od bloków.
Głośność UI i TEST PLAY/STOP nie sterują tym testem; to celowo jawna,
odrębna sekwencja sprzętowa, nie rzeczywisty player.

Osobne zadanie czeka poza ścieżką próbek na liczbę zakończonych buforów
z uwzględnieniem 4 wstępnych zerowych, potem wyłącza i usuwa kanał.
Timeout15s/błędny bufor/czas fill>=5ms kończy test błędem. Pomiar max_fill
obejmuje nasz callback renderowania przed zapisem statystyk, nie cały ISR,
latencję przerwań, jitter ani pomiar underrun. Wewnętrzna kolejka adresów
SDK dla blokującego write nie jest konsumowana; jej nadpisanie nie jest
metryką utraty próbek tego trybu callback. Przy opóźnieniu ISR może być
powtórzony bufor: bez pomiarów nie deklarujemy bezstratności ani braku trzasków.

Brak sieci/dekoderów/PCM FIFO/rzeczywistego volume ramp/playera/DSP.
Łagodne krawędzie sygnału syntetycznego nie dowodzą cichego start/stop
analogowego modułu ani obsługi underrun. Próby i pomiary pozostają OPEN.
