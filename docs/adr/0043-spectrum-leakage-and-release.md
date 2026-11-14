# ADR0043 — przeciek najniższych pasm i opadanie FFT

Data: 2026-10-31. Status: poprawka07f gotowa, odbiór OLED OPEN.

Użytkownik zgłosił podejrzenie braku okienkowania i migające/znikające
słupki przy mowie. Doprecyzował: pozostaje pusty wykres z linią, bez napisu
„Brak danych”. Ten widok oznacza ważny model o zerowych wysokościach,
a nie dowód expiry/przerw audio. Nowych logów max_fft/drop nie podano.

## Sprawdzenie okna

Hann N-1 było stosowane. Dodano niezależny test tonu poza binem20.37
dla1024/2048, porównujący odległy przeciek z prostokątnym oknem NumPy.
Test ujawnił artefakt w dolnych binach: odejmowanie zwykłej średniej bloku
przed Hann może wprowadzić niską składową po przemnożeniu przez okno.

Zmieniono DC removal na średnią ważoną Hann: sum(x*w)/sum(w). Implementacja
liczy najpierw różnice względem pierwszej próbki, dzięki czemu stały sygnał
pełnej skali znosi się dokładnie; suma float, bez software double na próbkę.
NumPy oracle używa matematycznie tej samej średniej ważonej. Teraz odległe
biny (odstęp>=8 binów od tonu) mają maksymalny przeciek ponad50 razy niższy
niż okno prostokątne dla obu długości testu. To test konkretnego sygnału,
nie gwarancja dla każdej audycji. Normalizacja/Nyquist/stereo bez zmian.

## Prezentacja

Dotychczas pojedyncza cicha klatka od razu zerowała słupki. Dodano wyłącznie
do FFT envelope po mapowaniu dB: natychmiastowe narastanie, ograniczone
opadanie jednej pełnej skali na320ms, zależne od czasu między pomiarami.
Pełne przejście do ciszy nadal daje zero, bez utrzymywania fikcyjnego szumu.
Mniejsze słupki opadają krócej. Nie jest to uśrednianie okien ani zmiana
RMS/peak; ich strojenie pozostaje odłożone. Takt10Hz bez zmian.
Envelope karmione tylko nowym ważnym FFT; nie odnawia timestampu.
STOP/generation/request resetują widmo, brak świeżych FFT przez200ms nadal
unieważnia model. Nie maskujemy długich luk i nie zmieniamy bufora audio.

Pasma są logarytmiczne: przy48kHz/N1024 zakres100Hz..4kHz obejmuje około
17 z24 kolumn. Sama duża szerokość obrazu nie dowodzi przecieku FFT ani
braku Hanna. Docelowe etykiety Hz/skala/strojenie wyglądu pozostają dalsze.

## Wyniki

305 testów hostowych PASS, NumPy oracle i nowy test przecieku wykonane.
Envelope/metering/native FFT harness ASan/UBSan PASS (LSan off).
Native ESP-IDF6.1 build PASS,0x15f3e0 (6% wolne), walidator i diff PASS.
Bez nowych zależności, alokacji, zmian ISR, flash/erase/eFuse/partycji.
Czas max_fft na S3 i widok po poprawce wymagają odbioru użytkownika.
