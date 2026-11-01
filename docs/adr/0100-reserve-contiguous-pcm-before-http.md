# ADR0100: rezerwacja ciągłego PCM przed HTTP/TLS

Data: 2026-11-01. Status: pozytywny krótki odbiór S3 z odsłuchem; długi soak OPEN.

## Dowód i diagnoza

Użytkownik zgłosił „Przestał się łączyć”. Log98f94c6a, SHA256
`a786c051a9e10f666c1eef5eea1924eab83f7d4c0335bd65f6ad43c7bd210fbc`,
2210 linii, zawiera dwa starty firmware3834551/ELFb2e60f14, z pełną
wcześniejszą diagnostyką. To wersja sprzed cichego profilu ADR0099,
nie dowód zachowania binarki59f732b.

Wi-Fi uzyskuje IPv4 w obu startach.28 awarii `stage=pcm_memory`, HTTP200,
`ESP_ERR_NO_MEM`, `ring=0 block=1 scratch=1`; dotyczą różnych formatów,
także AAC-LC i MP3 po drugim uruchomieniu. Serwer odpowiada i dane są
pobrane; blokadą odtwarzania jest alokacja ciągłego bufora PCM w internal.
32 bloki1924B =61568B. PSRAM nie zastąpi tego bufora: jest odczytywany
przez ISR z wymaganiem działania przy niedostępnym cache.

Bufor był przydzielany dopiero po nagłówkach HTTP i zebraniu preroll,
przed otwarciem dekodera. To zbyt późno względem alokacji readera/TLS
oraz innych zadań. Wolna pamięć sumaryczna nie gwarantuje ciągłego bloku.
Log nie ma free/largest dokładnie w chwili błędu, dlatego nie rozstrzygamy
wyłącznej fragmentacji względem chwilowego niedoboru całego internal heap.
Nie ma podstaw do diagnozy awarii Wi-Fi lub braku zaufanego ROOT CA
na podstawie tych28 wpisów.

Oddzielnie: gen22 ma `final_frame_crc` po zamknięciu TCP FIN. Nie uznajemy
tego za ten sam problem ani nie wyłączamy CRC/TLS, żeby go ominąć. Źródło
niepełnej końcowej ramki FLAC i rzeczywisty odbiór audio pozostają OPEN.

## Zmiana

Player rezerwuje ten sam ring w internal **przed uruchomieniem readera**,
po wycofaniu i zwolnieniu poprzedniej sesji. Dopiero sukces rezerwacji
pozwala otworzyć HTTP/TLS. Bez alokacji podczas nieaktywnego STA/AP/STOP.
Bufor pozostaje pusty do dotychczasowej inicjalizacji pipeline po preroll;
nie zmieniono rozmiaru, progów, opóźnienia audio ani formatu PCM.

Błąd rezerwacji zachowuje retry/backoff i podaje bytes/internal_free/largest,
bez URL lub sekretów. Błąd `http_open` zwalnia rezerwację natychmiast.
Po udanym open bufor należy do cyklu życia sesji: STOP/zmiana stacji/utrata
sieci/błąd źródła zwalniają go po wyłączeniu I2S i zakończeniu readera.
Obejmuje to anulowanie połączenia jeszcze przed rozpoznaniem kodeka.

Brak zmiany ISR, DSP, dekoderów, okna TCP, logowania okresowego lub TLS
verification. Nie zwiększamy buforów ani nie trzymamy ringa na stałe w BSS.
To poprawka kolejności alokacji; jej skuteczność na rzeczywistym układzie
pamięci S3 wymaga testu, nie jest gwarancją usunięcia dowolnego NO_MEM.

## Weryfikacja i odbiór

Nowe regresje rzeczywistego aktora playera:

- Reader symuluje brak dużego ciągłego bloku po swoim otwarciu; ring musi
  istnieć wcześniej. Próba obejmuje STOP, zmianę stacji, retry i usunięcie
  stacji oraz pełne zwolnienie pamięci. Kontrola negatywna: wcześniejszy
  kod nie przechodzi nowego warunku rezerwacji.
- Pierwsza rezerwacja zawodzi, następne HTTP open zawodzi, kolejne podejście
  gra: brak wycieków, poprawny backoff i STOP/cleanup.
- STOP i utrata sieci przed rozpoznaniem kodeka zwalniają ring; brak I2S
  lub dekodera przy samym oczekiwaniu na nagłówki.

19 regresji playera PASS, także ASan/UBSan (LSan off). Standardowy native
ESP-IDF6.1/ESP32-S3/dr build PASS. Pełna suita484 testy PASS;
`git diff --check` PASS. Bez flash/erase/NVS zmian przez agenta.

Odbiór: z katalogu projektu `idf.py -p /dev/ttyUSB0 flash`; wgrać zwykłe
build/, nie starszy obraz testowy. Sprawdzić AAC-LC Radio357, AAC+ RNŚ,
MP3 Antyradio, następnie FLAC/HLS i kilka przełączeń między nimi oraz STOP/
PLAY. Aktualne firmware nadal ma cichy UART; nazwa/ID przy zmianie stacji
oraz błędy pozostają. Nie trzeba usuwać zapisanych sieci ani playlisty.
Nowy runtime heap/largest, log błędów i odsłuch S3 OPEN.


## Wynik użytkownika — 2026-11-01

Log wklejony bezpośrednio w rozmowie, minicom13:29:28: firmware47e44c5,
ELF5fa63ece.9 wpisów Station changed od799 do178019ms — początkowe
Antyradio i8 zmian: TEST HLS64k-heaacv2-stereo, TEST AAC-LC stereo128k,
TEST Ogg Vorbis stereo64k, TEST Ogg Opus stereo64k, TEST FLAC native
lossless, powrót Antyradio, Antyradio AAC-LC, Radio357. W tym fragmencie
brak pcm_memory/NO_MEM, Connect/Stream failed, Underrun oraz resetu po boot.
To obserwacja podanego fragmentu; nazwy stacji nie są odczytem parametrów
rzeczywistego dekodera.

Na pytanie o poprawne granie wszystkich wybranych stacji, również FLAC
po kolejnych przełączeniach, użytkownik potwierdził: „Tak, wszystkie grały
poprawnie”. Pozytywny odbiór krótkiej próby zmiany stacji i odsłuchu
po poprawce alokacji PCM. Cichy UART zgodny z ADR0099: identyfikacja
firmware i zmiany stacji, bez okresowych raportów/śladów enkodera.

Nie zmieniano kodu po tym wyniku, nie powtarzano kompilacji i testów
regresji przy aktualizacji wyłącznie dokumentacji. Kontrola diff PASS.
Nie znamy runtime heap/largest ani szczegółowych liczników przy wyłączonej
diagnostyce. Długi soak, presja WWW, Wi-Fi flap i osobny końcowy CRC FLAC
z poprzedniego logu nadal OPEN. Nie uznajemy krótkiego odsłuchu za dowód
usunięcia wszystkich możliwych błędów pamięci lub źródła.
