# ADR0086 — odzyskiwanie ramek po utracie CRC, bez restartu całej sesji

Data:2026-11-01. Status: host PASS; odbiór S3 i źródło uszkodzenia OPEN.

## Nowy log

Załącznik ae2a00bd-14fe-444a-9993-024e386f9bf2, firmware218ecb2,
ELF44611b7b6, backend dr_flac-0.13.4, około196,7s. SHA256 pliku:
`b1b287ad486637e030e5af6ee21143b80c5d324b675b273f48250ea45ca436e7`.
Użytkownik potwierdził słyszalne przerwy.72 raporty Underrun, gen3/4/5:
31/33/8. Dwa odrzucenia w61,187s i108,477s: status-6,
frame_limit_no_boundary, scan131072, bytes131269/135117,
CRC16dab4/d58c. Reader nadal HTTP200/ESP_OK/ended0.

Nagłówki ff f8 59 1c / ff f8 59 ac opisują4608 próbek,44,1kHz,
stereo24bit. Sam fakt osiągnięcia limitu nie dowodzi prawidłowej dużej
ramki: parser nie odnalazł granicy przy pasującym CRC.18 interwałów CPU
średnio40,49%/40,64%, bez dowodu stałego nasycenia. Nie zwiększamy limitu
ani nie przypisujemy uszkodzenia serwerowi, Wi-Fi, PSRAM lub bibliotece.
Nadal brak pełnego odrzuconego bufora z urządzenia.

Użytkownik potwierdził przygasanie FLAC/1500k tylko w wierszach bargraphu,
także po szachownicy; reszta ekranu i inne oprogramowanie na tym sprzęcie OK.
Test pełnego HOME sprawdza, że piksele pola FLAC/1500k są bajtowo identyczne
przy różnych widgetach i ich poziomach. Profil OLED badany osobno w ADR0087.

## Niezależna próba źródła

Z komputera pobrano80s publicznej próbki Radio Mast z Icy-Metadata:1,
interval16384. Raw17683923B, audio17682348B. SHA256 raw:
`61e8d5fedb0877cc1224a458acd9f2c49e77079f2536618735b1bfd2ef2983d9`.
Ograniczenie czasu curl zakończyło pobieranie kodem28; to zamierzony koniec
testu żywego strumienia, nie wykryty błąd serwera. Dane zostają wyłącznie /tmp.

Prawdziwy C ICY przy porcjach1/1024/4096B dał identyczne audio jak niezależny
stripper. Parser znalazł956 pełnych ramek, największa24857B;18543B końcówki
niepełnej pominięto w próbie PCM. dr, micro i micro32 z porcjami4096 oraz
deterministycznie zmiennymi1..4096B: PCM identyczny z FFmpeg, także reset.
Próbka z innego czasu/trasy nie odtworzyła błędu z urządzenia.

## Zmiana

Dotychczas po jednym błędzie CRC całego prefiksu parser nigdy nie wracał
do granic kolejnych dobrych ramek. Osiągał128KiB i wymuszał reconnect,
choć źródło mogło zawierać już poprawne audio. Dodano ograniczony fallback:

- równolegle zapamiętany kandydat z poprawnym CRC8 nagłówka;
- pełna ramka kandydata musi mieć poprawne CRC16 oraz następny poprawny
  nagłówek albo koniec EOS; sam sync ani sam nagłówek nie wystarczają;
- kandydat jest tylko zapamiętany: wolno pominąć prefiks dopiero przy
  dotychczasowym limicie128KiB lub niepoprawnym CRC na EOS. Poprawna ramka
  może zawierać kompletne ciągi FLAC wewnątrz verbatim PCM. Nie można
  porzucać jej wcześniej tylko dlatego, że znaleziono taki ciąg;
- skipped_size odrzuca uszkodzony prefiks, resetuje stan CRC i przekazuje
  zweryfikowany fragment ponownie normalnemu parserowi/dekoderowi;
- log flac_parse Recover: skipped/validated_frame/prefix_crc16. Bez dobrego
  kandydata pozostaje dotychczasowy Reject i retry.

W dr poprawiono opróżnianie cache przy odzyskaniu na EOS: ostatni bajt
wejścia jest już skopiowany, ale caller trzyma go jako pending aż wszystkie
odkryte ramki zostaną zdekodowane. Bez tego znikały końcowe dobre ramki.

Nie wyłączono CRC, nie zmieniono upstream, buforów, progów, transportu ani
I2S. Dodatkowy stan jest stałego rozmiaru; dodatkowe CRC tylko przy aktywnym
kandydacie. Micro używa własnego parsera biblioteki i nie dostaje fallbacku.
To ograniczenie skutków uszkodzenia, nie naprawa jego źródła. Zwykłe niedobory
PCM oraz krótkie zaniki wynikające z odrzuconego audio mogą nadal wystąpić.

## Weryfikacja

Przed zmianą nowy test uszkodzonego CRC/ubytek/dodatkowe bajty/dwie złe
ramki kończył się12 razy statusem-6. Po zmianie parser zachowuje wszystkie
pozostałe ramki. Realny dr porównany z FFmpeg po usunięciu uszkodzonego
bloku: porcje0/1/7/257/4096/65536, dwa cykle/reset, dokładny PCM PASS.
Dodatkowy korpus verbatim z kompletnymi wewnętrznymi ramkami w payload
nie jest przedwcześnie pomijany. Błędna ostatnia ramka bez fallbacku nadal
kończy się błędem.427 host tests i ASan/UBSan testu recovery dr PASS
(LSan off w środowisku ptrace); native build w STATUS. Agent nie flashuje.

Odbiór: ten sam dr i stacja; porównać Recover/Reject, Underrun i słyszalne
przerwy. Nie zaliczać całości po samym zniknięciu reconnectów. W razie
powrotu odrzuceń potrzebny dokładny bufor z S3 lub kontrolowane źródło LAN.
