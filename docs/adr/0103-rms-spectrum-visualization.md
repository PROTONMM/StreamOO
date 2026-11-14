# ADR0103 — czwarta wizualizacja: widmo RMS pasm

Zastąpione przez ADR0105 na polecenie użytkownika2026-11-14: RMS widma
usunięte, oba tryby to maksimum pasma−45..0dBFS/gamma2, drugi z peak hold.
Poniższy opis jest historyczny.

Aktualizacja2026-11-14: odbiór użytkownika „nie ma różnicy”. ADR0104
zastępuje wspólną skalę osobną skalą−45..0dBFS/gamma2 dla trybu RMS.
Poniższa wspólna skala i wynik488 testów opisują pierwszą wersję.

Data:2026-11-14. Status:zaimplementowane na polecenie użytkownika;
odbiór OLED i wpływu na runtime OPEN.

## Powód i decyzja

Obecne widmo maksimum użytkownik ocenia jako zbyt intensywne. Zamiast
zastępować je lub jednocześnie zmieniać skalę, dodajemy czwarty tryb RMS
pasma. Zachowujemy identyfikatory0/1/2 i dodajemy3, zgodne ze starym blobem
preferencji. Cykl enkodera obsługuje cztery tryby i duże delta w obie strony.
Oryginalny pokaz home_probe pozostaje historycznym pokazem trzech trybów;
normalny interaktywny adapter i testy renderera obsługują cztery.

## Znaczenie RMS

Dotychczas A[k] to amplituda prążka po Hann/DC removal i połączeniu mocy
stereo. Stare pasmo=max(A[k]), nowe=sqrt(sum(A[k]^2)/liczba_prążków).
To RMS zbioru amplitud widmowych z istniejącą kalibracją tonu, a nie RMS
czasowy sygnału ani suma energii pasma. Dla jednego prążka oba wyniki są
takie same. Szerokie pasmo tłumi izolowany ton względem maksimum — celowy
kompromis wynikający z prośby o spokojniejszy wykres. L/R w przeciwfazie
nadal nie kasują się. Poziomy liniowe Q15 są saturujące.

Oba widma mają identyczny rendering,24 pasma logarytmiczne, dotychczasową
skalę−60..0dBFS/gamma1,5 i osobny envelope320ms. Nie obiecujemy płynniejszego
ruchu w czasie wyłącznie z powodu agregacji RMS; to zmiana sposobu obliczania
poziomu pasma, nie dodatkowa średnia czasowa.

## Architektura i koszty

Jedna FFT daje oba zestawy pasm. Nowy wynik to48B w workspace PSRAM;
snapshot meteringu rozszerzony o48B plus ewentualny padding kopii, drugi
stan envelope na stosie workera. Dodatkowe sumowanie/pierwiastki/mapowanie
wyłącznie w audio_levels, bez dodatkowej FFT, alokacji w analyze ani zmian
callbacku I2S. Worker nadal core0/priority4, droppable tap. Runtime czasów
FFT/CPU i zapasu stosu wymaga nowego pomiaru S3; nie zmniejszamy stosów.

Włączenie FFT obejmuje oba tryby widma; dla bargraphu i wskazówek pozostaje
wyłączone. Snapshot zawiera oba niezależne zestawy, UI wybiera właściwy.
Expiry i reenable zerują oba; zmiana trybu nie utrzymuje poprzedniej
wizualizacji przy brakujących danych. Brak nowych zależności/zmian licencji.

## Weryfikacja

-488 testów hostowych PASS, bez skipów. NumPy oracle porównuje oba rodzaje
pasm dla1024/2048, tonów/różnych rate/impulsu/szumu. Osobno wide-band tone,
jednoprążkowa równość, RMS<=peak i cisza. sc16/float RMS w tolerancji82 Q15;
ANSI oracle nie dowodzi wydajności instrukcji S3.
- Preferencje:404 prawidłowe pary, stare ID i nowe3, nieprawidłowe4 odrzucone.
- Sterowanie/renderer:cztery tryby, wrap, clipping i brak wycieku poprzedniego
trybu przy brakujących danych; metering:oba zestawy wygasają.
- ASan/UBSan:11 spectrum,9 metering,15 controls i stress renderera PASS;
LSan wyłączony, bez deklaracji testu wycieków.
- Native ESP-IDF6.1 build PASS,0x173500B,90% app wolne.
- Walidator sdkconfig --platform i git diff --check PASS.

Bez flash/erase. Odbiór:ten sam utwór/mowa w widmie0 i3, przełączanie
oboma kierunkami bez zmiany VOL/restartu audio, zapis3 po5s i power-cycle.
Ocenić słupki, ciągłość audio oraz diagnostycznie CPU/stos audio_levels.
