# ADR0058 — ramki24-bit FLAC większe niż domyślne okno parsera

Data:2026-10-31. Status: poprawka gotowa; odbiór S3 OPEN.

Użytkownik potwierdził start live FLAC po ADR0056, potem zgłosił przerwy.
Log: HTTP200,44,1kHz/stereo24, compressed blisko256KiB, PCM32, invalid0,
więc brak dowodu na niedostateczny dopływ sieci. Parser raportuje
max_frame20480/Not found frame, potem vendor-8/BUFF_NOT_ENOUGH,
needed82944 przy scratch65536 i restart sesji. Tap drop/gaps nie są
niedoborem PCM i nie wyjaśniają błędów parsera.

Pomiar hosta:3MiB publicznego strumienia,179 pakietów, po4608 próbek,
24-bit stereo44,1kHz; największy skompresowany pakiet25839B. To przekracza
20480B. W bootstrap STREAMINFO pole max_frame_size było zerem (nieznane),
co jest legalne w FLAC, ale powoduje wybór za małego fallbacku w parserze
Espressif2.6.2. Pierwsza poprawka nie uwzględniała tego ograniczenia.

## Zmiana

Dla wygenerowanego STREAMINFO ustawiamy jawny limit obsługiwanego okna
128KiB zamiast zera. To limit implementacji, nie zmierzony największy
rozmiar całej audycji ani estymacja bitrate; większe ramki pozostają poza
zakresem. Oryginalnych metadanych plików FLAC nie zmieniamy.

Scratch PCM również128KiB w PSRAM, żeby pomieścić zgłoszone82944B bez
restartu; nie twierdzimy, że z samego logu znamy powód wielkości tego
pakietu wyjściowego. Zwiększenie PCM nie zastępuje poprawki okna wejścia.
Alokacja nadal przed uruchomieniem I2S, cały pump w PSRAM. Stały limit,
bez realloc w decode, bez zmian ring/ISR/gain/DSP/partycji/playlisty.
Dodatkowe64KiB PCM i większy cache parsera należy ocenić na sprzęcie.

## Testy

Oryginalny deterministyczny szum24-bit stereo zakodowany hostowym FFmpeg
w FLAC daje ramki>20480B. Po usunięciu metadanych i bootstrap maxframe
obejmuje te ramki; niezależny FFmpeg dekoduje identyczny PCM. Test dużego
pakietu PCM82944B sprawdza rzeczywisty live adapter i konwersję24→32,
nie wykonuje binarnego dekodera Espressif na hoście.
357 testów hostowych, header/live ASanUBsan (LSan off), native build,
GPIO validator i diff check PASS. Szczegóły STATUS.

Po zwykłym flash sprawdzić ten sam FLAC przez kilka minut, również po
zmianie utworu. Oczekiwany parser max_frame131072 zamiast20480; brak
Not found frame i restartów. Heap/stack/low/hard do odbioru S3.
ROOT CA import odłożony przez użytkownika; OTA nadal wyłączone.
