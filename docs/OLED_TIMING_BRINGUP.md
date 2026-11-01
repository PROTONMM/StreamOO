# Pulsowanie i poziome prążki OLED

2026-11-01. Zgłoszenie: lekkie pulsowanie oraz poziome prążki na bargraph
i spectrum. Przyczyna OPEN; nie potwierdzono jeszcze, czy widać je gołym
okiem, na nieruchomym obrazie, ani czy zależą od zasilania.

## Co wynika z bieżącego kodu

- `main/display_smoke.c`: interaktywny UI ma okres 40 ms, czyli docelowo
  25 klatek/s. Jest to termin zadania, nie zmierzony FPS pod obciążeniem.
- `ssd1322_protocol_present()` zapisuje całe 8192 B do widocznego GDDRAM.
  Nie synchronizuje początku zapisu ze skanowaniem panelu.
- Przy skonfigurowanych 10 MHz sam payload potrzebuje co najmniej
  6,5536 ms; dochodzą komendy, kopiowanie i planowanie zadań. Payload to
  204800 B/s i 16,384% czasu magistrali przy dokładnie 25 klatkach/s.
  Te obliczenia nie są pomiarem czasu transferu ani obciążenia CPU.
- Canvas jest czyszczony i rysowany w RAM, następnie przesyłany. Nie ma
  osobnej transmisji pustej klatki ani AE/AF przy każdym odświeżeniu.
  AE/AF służą startowi oraz wygaszaniu/wybudzeniu po STOP/PLAY.
- Spectrum ma wypełnienie GS8 i szczyt GS15; bargraph GS10 i peak GS15.
  Większe jednolite pola półtonów mogą ujawniać artefakty słabo widoczne
  na małych jasnych literach. Nie stosujemy czasowego ditheringu w rendererze.
- Zegar panelu jest niezależny od SPI/FPS aplikacji: B3=0x50 (poziom
  oscylatora 5, dzielnik 1), MUX=64, B1=0x74, tabela szarości B9.
  Nie zmierzono rzeczywistej częstotliwości skanowania tego modułu.

## Interpretacja i źródła

Dokumentacja SSD1322 rev.1.2 rozdziela zapis GDDRAM od zegara skanowania;
sekcje 8.5, 8.7 i 8.8 opisują multipleksowanie wierszy i jasność PWM.
Zbyt wolne skanowanie może migotać. Sama redukcja FPS aplikacji nie
zmniejsza częstotliwości tego oscylatora.
[Dokumentacja Solomon Systech, s.23–29, 34 i 45](https://newhavendisplay.com/content/app_notes/SSD1322.pdf).

Newhaven opisuje problem tearingu przy dynamicznym obrazie SSD1322
i sugeruje dobór B3 względem renderowania. To inny moduł niż moduł
użytkownika; nie przenosimy jego parametrów panelu do StreamOO.
[Odpowiedź wsparcia producenta](https://support.newhavendisplay.com/hc/en-us/community/posts/22793159566871-vsync-on-NHD-3-12-25664UCW2?sort_by=votes).

Hipotezy do rozdzielenia: zapis nowej klatki podczas skanowania (tearing),
skanowanie/PWM półtonów, zakłócenia lub tętnienia zasilania. Kamera może
ujawniać dodatkowe pasy, których nie widać gołym okiem. Opis objawu sam
nie identyfikuje żadnej z tych przyczyn.

## Eksperyment na sprzęcie

1. Na obecnym firmware obejrzeć gołym okiem nieruchome logo po resecie
   i ruchome wizualizacje. Logo jest wysyłane raz i pozostaje bez kolejnych
   transferów podczas oczekiwania startowego; test nie wymaga nowego buildu.
   Zanotować oddzielnie pulsowanie jasności i poziome prążki, także to,
   czy dotyczą jasnego peaku, czy tylko półtonowego wypełnienia.
2. Jeśli potrzebny dłuższy nieruchomy obraz: istniejący skończony test
   z `STREAMOO_ENCODER_UI_TEST=n` zatrzymuje każdy wzór na 3 s bez kolejnych
   zapisów RAM. Zmiana dotyczy osobnego buildu diagnostycznego; zależne
   testy/sterowanie audio mogą być wyłączone. To izolacja panelu, nie
   porównanie pod identycznym obciążeniem radia. Nie usuwać sdkconfig/NVS.
3. Prążki na stałym obrazie bez transferów wykluczają zmianę zawartości
   klatek jako jedyną przyczynę. Wtedy badać skanowanie/PWM i zasilanie
   modułu. Sam taki wynik nie wskazuje, który z nich odpowiada za problem.
4. Jeśli objaw dotyczy ruchu: przygotować kontrolowane porównanie 25/10 FPS
   z tym samym sygnałem, jasnością, SPI i B3. Zmierzyć czas present i odstępy
   klatek. Sprawdzić także identyczny nieruchomy wzór wysyłany cyklicznie
   oraz wysłany tylko raz. Redukcja FPS jest eksperymentem, nie dowodem naprawy.
5. Dopiero osobno porównać B3 po ustaleniu zachowania stałego obrazu.
   Nie zmieniać jednocześnie kontrastu, gamma, precharge i napięć panelu.

Wynik obserwacji użytkownika i pomiary SPI/zasilania: OPEN.
Na tym etapie nie zmieniono firmware ani parametrów panelu.
