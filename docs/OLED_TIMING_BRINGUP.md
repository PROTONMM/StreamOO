# Pulsowanie i poziome prążki OLED

Aktualizacja2026-11-01: użytkownik potwierdził, że ostatnia kompilacja
rozwiązała problem wyświetlacza po zmianie timingów ADR0087. Nowego logu
z hashem nie podał. Optymalizacja transferów ADR0088 jest osobnym krokiem:
porównanie pikseli i wysyłanie zakresu wierszy. Test pięciu faz poniżej nadal
używa wymuszonego present, w tym REPEAT.

2026-11-01. Zgłoszenie: lekkie pulsowanie oraz poziome prążki na bargraph
i spectrum. Użytkownik potwierdził widoczność gołym okiem: dół pulsuje
w rytm wizualizacji, wyżej czasem na ułamek sekundy przygasa napis.
Użytkownik potwierdził poprawny wygląd logo i brak pulsowania wskazówek.
Przygasa FLAC1500k tylko na wysokości bargraphu, reszta obrazu i inne
oprogramowanie na tym sprzęcie OK. Zasilanie nie jest główną hipotezą.
Przyczyna OPEN. ADR0087 zmienia wyłącznie timing B3/B1 z0x50/0x74 na
0x91/0xE2, profil do potwierdzenia na module. Nie zwiększa prądu/napięć.
ADR0084 zmienia zwykły bargraph
na szachownicę z obrysem; pięć syntetycznych faz poniżej pozostaje bez zmian,
aby zachować oryginalne wzorce kontrolne. Wynik nowego wyglądu OLED OPEN.

## Nowy test rozdzielający przyczyny — ADR0083

Opcja `STREAMOO_LCD_TIMING_PROBE` jest domyślnie wyłączona. Po włączeniu
uruchamia się raz po resecie, po przejściu playera sieciowego w PLAYING
i zakończeniu logo. Radio oraz WWW pracują dalej. Diagnostyka zastępuje UI
przez 40 s; dowolne zdarzenie enkodera kończy test i zostaje zużyte.
Potem wraca zwykły UI. Test nie powtarza się przy zmianie stacji/PLAY.
Stały napis i cztery pola GS6/8/10/15 u góry są referencją jasności.

| Faza w logu oled_probe | Czas | Zawartość / transmisja |
|---|---|---|
| HOLD | 8 s | Stałe słupki, jedna klatka, potem brak transferów OLED |
| REPEAT | 8 s | Dokładnie te same piksele, pełna klatka co 40 ms |
| LOAD 25fps | 8 s | Zmienna wysokość słupków, okres 40 ms |
| LOAD 10fps | 8 s | Ta sama funkcja animacji, okres 100 ms |
| MOVE | 8 s | Ruch w poziomie przy stałej liczbie pikseli każdego półtonu, 40 ms |

To wzór syntetyczny, niezależny od amplitudy muzyki. Wybierz wcześniej
stabilną stację MP3, aby ograniczyć wpływ ponownych połączeń/dekodera.
Test nie reprodukuje dokładnie kosztu renderowania zwykłego UI ani
identycznego obciążenia Wi-Fi między fazami. Log pokazuje rzeczywistą
liczbę transmisji, czas fazy oraz średni/maksymalny czas present. Czas
rysowania w RAM nie wchodzi do metryki present. Podane FPS są zadanym
okresem, rzeczywiste tempo można obliczyć z frames/elapsed_us.

Przykład dla aktualnego wariantu dr (bez zmiany backendu):

```sh
. /home/mariuszk/.espressif/v6.1/esp-idf/export.sh
idf.py -B build/flac-dr -D SDKCONFIG="$PWD/sdkconfig.flac-dr" menuconfig
# StreamOO Configuration -> Hardware: SSD1322
# zaznacz OLED pulsation experiment after playback starts (40 seconds)
python3 tools/build_flac.py dr flash --port /dev/ttyUSB0
```

Po teście wyłącz opcję w tym samym menu i ponownie zbuduj/wgraj firmware.
Konfiguracja główna sdkconfig oraz NVS nie wymagają usuwania.

Interpretacja, bez przesądzania wyniku:

- HOLD pulsuje: aktualizacja obrazu nie jest jedynym powodem.
- REPEAT gorzej niż HOLD: badać wpływ SPI/zapisów RAM, także elektryczny;
  zawartość obrazu oraz liczba świecących pikseli nie zmieniają się.
- LOAD pogarsza stały napis bardziej niż MOVE: podejrzenie zależności od
  obciążenia panelu/zasilania; wymaga pomiaru napięć, nie dowodzi usterki PSU.
- Zmiana 25 na 10 FPS wpływa na prążki: istnieje zależność czasowa, ale
  nadal trzeba rozdzielić skanowanie od zakłóceń związanych z transferami.

Przekaż log `oled_probe` i obserwacje osobno dla pięciu faz. B3, precharge,
gamma, kontrast, prąd, SPI i bufory audio nie są zmieniane przez test.

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
- Zegar panelu jest niezależny od SPI/FPS aplikacji. Od ADR0087: B3=0x91
  (poziom oscylatora9, dzielnik2), MUX=64, B1=0xE2, tabela szarości B9.
  Pierwotny eksperyment ADR0082/83 używał B3=0x50 i B1=0x74.
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

Wyniki pięciu faz i pomiary SPI/zasilania: OPEN. Pierwotna analiza ADR0082
nie zmieniała firmware; ADR0083 dodaje wyłącznie opcjonalną diagnostykę.
