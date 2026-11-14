# ADR0085 — wynik dr_flac na S3 i dokładny powód odrzucenia ramki

Data: 2026-11-01. Status: test dr odebrany; stabilność FAIL, przyczyna OPEN.
Odsłuch niepełny: użytkownik nie słuchał przez cały test. Nie przypisujemy
każdemu ostrzeżeniu konkretnej słyszalnej przerwy ani nie zaliczamy odsłuchu.

## Dowód wejściowy

Załącznik85dc8a81-ceaf-4708-bbf4-42d65e5d86fb, 3307 linii, około889,6 s
od boot. SHA256 pliku:
`5a935718446ed33ca93c51fe5db24ca9c17f0f480365a815253ff08fc4117a20`.
Firmware73fa94e, ELF1236eebb5, stacja TEST FLAC native lossless.
Backend potwierdzony przy otwarciu oraz w Decode perf: dr_flac-0.13.4.
To wersja sprzed zmiany wyglądu bargraphu ADR0084, więc log nie służy do
oceny szachownicy. Surowego logu z danymi sieci lokalnej nie dodajemy do repo.

## Trzy niezależne zjawiska

1. Gen1..5 przed odtwarzaniem: TLS-0x2700 i HTTP_CONNECT. Przypięty mbedTLS
   definiuje ten kod jako X509_CERT_VERIFY_FAILED. Log nie pozwala ustalić
   konkretnej przyczyny weryfikacji certyfikatu. Pierwsze otwarcie dr w41,367s.
2. Gen6/7/8 kończą się błędem decode vendor=-1001, w576,967/636,437/685,337s.
   W każdym przypadku Reader ended=0, error=ESP_OK, HTTP200, read1024.
   W adapterze dr -1001 oznacza błąd streamoo_flac_parse(), przed decode_frame
   i drflac_read_pcm_frames_s32. Nie jest to kod biblioteki dr_flac ani dowód
   uprzedniego zerwania Wi-Fi/TCP. Player po błędzie zamyka sesję i łączy się
   ponownie. Dokładny parser status i problematyczne bajty w starym logu nie
   zostały zachowane; nie można przypisać błędu samemu CRC lub limitowi.
3. Łącznie121 raportów Underrun, niezależnie od trzech błędów parsera.
   Compressed wielokrotnie schodzi do1B; przy części ostrzeżeń cache PCM=0,
   rosną wait_empty i wait_busy. Są też ostrzeżenia z niezerowym compressed
   i cache. To snapshot ownera po zdarzeniu, nie dokładny stan w ISR.
   Potwierdza niedobory i potrzebę zbadania dopływu/buforowania/terminowości,
   nie identyfikuje pojedynczej przyczyny w całej ścieżce.

| Sesja | Od otwarcia dr do błędu/końca logu | Raporty Underrun | Zakończenie |
|---|---:|---:|---|
| gen6 |535,60s|15|parser -1001|
| gen7 |55,17s|33|parser -1001|
| gen8 |44,77s|25|parser -1001|
| gen9 |200,13s|48|nadal gra na końcu logu|

Nie sumować low z okresowych snapshotów: liczniki zerują się przy nowym
pipeline, a snapshot CONNECTING może jeszcze zawierać dane poprzedniej sesji.
121 to liczba ostrzeżeń z przypisanym generation w tym konkretnym logu.

## Wydajność i granice wniosków

84 interwały CPU od pierwszego raportu podczas grania do końca logu:
CPU0 średnio41,36%, CPU1 43,98%, task player38,92% jednego rdzenia.
Maksima interwałów CPU0/1:49,3%/50,6%. Dane obejmują różne utwory,
przełączenia widżetów i krótkie reconnecty; to nie kontrolowany benchmark.
Nie dowodzą braku chwilowych opóźnień schedulera/cache lub przekroczenia
terminu zasilenia FIFO. Nie wyznaczamy speedupu względem innych logów micro.

Dla samej gen6, po pominięciu pierwszych10s od otwarcia dr,52 próbek:
CPU0/1 średnio40,8%/44,5%, player39,7%. Największe codec_max w całym
logu55,523ms, block_max65,216ms. To czasy ścienne, nie sam czas CPU.
Brak dowodu na stałe nasycenie rdzeni; wymiana biblioteki nie usunęła przerw.
Minimalny raportowany heap internal29500B, PSRAM free około6,76MB;
brak raportu błędu alokacji lub panic. Nie deklarujemy pełnego soak pamięci.

FFT pracuje na ESP-DSP SIMD. Późniejsze valid=0/stojący licznik frames
nie wystarczają do diagnozy zacięcia: w logu są przełączenia widżetów,
a analiza FFT jest celowo wyłączana poza widmem. Niedobory występują również
po tych przełączeniach. Nie ma podstaw do przypisania całości FFT.

## Decyzja i nowa diagnostyka

Zachować backend, transport, bufory i progi. Zamiast zgadywać powód -1001
dodać log tylko na ścieżce odrzucenia w streamoo_flac_parse:

- rzeczywisty status i nazwa konkretnego warunku, np. frame_header,
  frame_limit_no_boundary, final_frame_crc, cache_length_regressed;
- rozmiar cache, pozycja scan, CRC16, pozostałe metadane i BOS/EOS;
- pierwsze maksymalnie16 bajtów odrzuconego bufora w osobnej krótkiej linii.

Nie logować zwykłego DATA_NOT_ENOUGH ani poprawnych ramek. Nie wyłączać CRC,
nie pomijać uszkodzonych danych i nie zmieniać obsługi błędu/retry.
Logowanie działa w tasku dekodera po błędzie, poza ścieżką wydawania I2S.
Nie zapisuje audio do flash ani nie dodaje alokacji w poprawnym przetwarzaniu.

Nowy log zawęzi przyczynę;16 bajtów nie zastąpi pełnej ramki przy potrzebie
odtworzenia CRC. Kolejny etap zależy od konkretnego odrzucenia: kontrolowany
plik LAN z identycznym PCM/porcjami lub przechwycenie dokładnych bajtów
błędnej ramki. Nie zwiększamy równocześnie buforów i nie zmieniamy kodeka.
Wyniki host/build w STATUS; brak flash przez agenta i brak deklaracji naprawy.

## Dodatkowe sprawdzenie hipotezy serwera / startu TLS

Na pytanie użytkownika o serwer sprawdzono bieżący endpoint z playlisty
testowej. 2026-11-01 około08:40UTC: HTTPS302 z streams.radiomast.io do
audio-edge-3mayu.fra.h.radiomast.io, następnie HTTP200 audio/flac, icy-br1500.
OpenSSL przez Python ssl z CAfile DOKŁADNIE przypiętego SDK6.1: po dwie
zweryfikowane próby obu hostów, wszystkie PASS, około318–321ms i70–72ms.
Leaf pierwszego hosta ważny20.09–19.12.2026, edge11.09–10.12.2026.
To stan bieżący i trasa komputera, nie dowód poprawności serwera w chwili
wcześniejszego logu ani identycznej trasy DNS/łańcucha na urządzeniu.

Kod sieci uruchamia SNTP dopiero po GOT_IP, wait_for_sync=false, callback
ustawia time_synced bez logu. Player nie czeka na czas przed otwarciem
HTTPS; HAVE_TIME_DATE=y. Niepoprawny czas przed SNTP jest zatem silną
hipotezą początkowych błędów -0x2700, nie potwierdzonym wynikiem logu.
Do rozstrzygnięcia potrzebny UTC/time_synced i pełne tls_flags w momencie
błędu. Nie wyłączamy sprawdzania dat ani nie przypisujemy winy serwerowi.

[Radio Mast](https://www.radiomast.io/reference-streams) opisuje tę próbkę
jako native FLAC16/stereo44,1kHz, VBR300–1500kbps. Zależność od utworu może
więc wynikać z zapotrzebowania na transfer/bufor, ale nie dowodzi awarii CDN.
[Espressif](https://docs.espressif.com/projects/esp-techpedia/en/latest/esp-friends/advanced-development/protocol/mbedtls-troubleshooting.html)
klasyfikuje -0x2700 jako błąd weryfikacji certyfikatu, nie timeout serwera.
W tym dodatkowym sprawdzeniu nie zmieniono kodu i nie wykonano flash.
