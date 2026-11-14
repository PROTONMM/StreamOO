# ADR0104 — osobna skala widma RMS

Zastąpione przez ADR0105 na polecenie użytkownika2026-11-14: RMS widma
usunięte, oba tryby to maksimum pasma−45..0dBFS/gamma2, drugi z peak hold.
Poniższy opis jest historyczny.

Data:2026-11-14. Status:zaimplementowane po uwadze użytkownika;
wygląd nowej skali na OLED OPEN. Uzupełnia ADR0103.

## Problem i dowód

Użytkownik:„nie ma różnicy” między pierwszym i czwartym widmem.
Identyfikator wgranego obrazu nie został podany. Inspekcja kodu potwierdza
osobne rms_bands i wybór ID3; nie przypisujemy obserwacji błędnemu flashowi.
Przy48kHz/N1024 pierwsze9 pasm ma po1 prążku: RMS amplitud jest wtedy
matematycznie identyczny z maksimum. Wcześniejsza skala obu trybów była
taka sama−60..0dBFS/gamma1,5. Test kontrolowanego wielotonu potwierdził
identyczne wysokości pierwszych9 pasm; różnica w szerokich pasmach może
być słabo widoczna. Nie był to pomiar rzeczywistej audycji użytkownika.

## Decyzja

Dodajemy osobną funkcję prezentacji wyłącznie RMS:
`u=clamp((20*log10(A/32767)+45)/45,0,1)`, wyjście=`round(u*u*32767)`.
Zero zwraca0 bez logarytmu, wartości>=32767 saturują. W zwykłym widmie
zachowujemy dotychczasową funkcję−60..0dBFS/gamma1,5. Bez zmian agregacji,
DC/Hann/FFT, miksowania stereo, envelope, taktowania, danych PCM lub I2S.
Brak nowych bibliotek/alokacji/formatów NVS i drugiej FFT.

Dla−40/−30/−20/−10dB wysokość na31px wynosi0/3/9/18px zamiast5/10/16/23px.
Słabe składowe będą częściej niewidoczne; to celowe zmniejszenie czułości.
Nie zmieniamy RMS amplitud prążków w fizyczny RMS czasowy/energię pasma.

## Weryfikacja

- Test skali sprawdza wszystkie65536 wartości: monotoniczność, saturację,
zero i poziom nigdy wyższy niż dotychczasowa skala; cztery referencje
sprawdzają wynik na wysokości rzeczywistego widgetu31px.
- Test rzeczywistego workera meteringu:ton−20dB w binie3 (pasmo1-bin),
oba snapshoty poprawne; poprzednie widmo16px, nowe RMS9px. To sprawdza
połączenie nowej skali z publikowanymi danymi, nie tylko samą funkcję.
-490 testów hostowych PASS, bez skipów. ASan/UBSan:7 spectrum_stream,
10 metering PASS; LSan off. Walidator sdkconfig --platform PASS.
- ESP-IDF6.1 native build PASS, obraz0x173590B,90% app wolne.
- git diff --check PASS.

Agent nie flashował. Odbiór:czwarty tryb po wskazówkach, porównanie z
pierwszym na tej samej audycji, czytelność basu/mowy i ciągłość audio.
