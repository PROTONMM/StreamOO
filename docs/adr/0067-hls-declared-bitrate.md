# ADR0067 — bitrate wariantu HLS na istniejącym wskaźniku

Data:2026-10-31. Status: implementacja/host/build PASS; odbiór OLED OPEN.

Użytkownik: HLS nie pokazuje bitrate. Dotychczas ADR0049 czytał icy-br;
HLS pomijało deklarację wariantu i zerowało bitrate przy starcie. Publiczne
mastery RadioMast z ADR0065 mają np. BANDWIDTH=128000 lub64000.

Parser zachowuje bitrate wybranego wariantu: poprawne AVERAGE-BANDWIDTH,
a przy braku/błędzie BANDWIDTH. Średnia musi być dodatnia i<=peak. Selekcja
wariantu pozostaje według dotychczasowego peak BANDWIDTH, bez zmian ABR.
Oba pola mają jednostkębit/s, konwersja dointegerkb/s z zaokrągleniem do
najbliższej wartości. Zgodnie z RFC8216 BANDWIDTH to deklarowany peak,
AVERAGE-BANDWIDTH średnia segmentów; mogą obejmować narzut kontenera.
Źródło: https://www.rfc-editor.org/rfc/rfc8216.html#section-4.3.4.2 .
Nie jest to pomiar VBR/TCP ani wyliczenie z sample rate czy bufora.

Reader publikuje wynik przy wybraniu master→URI; media reload i nagłówki
kolejnych segmentów nie zerują ani nie nadpisują ustalonej deklaracji.
Zagnieżdżony master może zastąpić/wyczyścić ją zgodnie ze swoim wyborem.
Odrębny EXT-X-MEDIA audio URI nie dostaje bitrate innego wariantu, ponieważ
nie ma własnej deklaracji. Przy braku wartości HLS pozostaje możliwy fallback
icy-br segmentu; bez wiarygodnej deklaracji nadal0/--, bez zgadywania.

Istniejący player i OLED używają bitrate_kbps dopiero przy znanym PCM/kodeku;
STOP/retry/zmiana sesji nadal czyszczą UI. Format/API/UI bez nowych elementów:
TEST HLS128k→128k,64k→64k, gdy tak deklaruje master. Nie wyciągamy profilu
HE-AAC z nazwy stacji ani bitrate. HLS queryfix, diagnostyka i bufory bez zmian.
Brak nowych zależności/kodu zewnętrznego, GPIO/partycji/NVS/playlisty bez zmian.

380 testów hostowych PASS, HLS/HTTP ASanUBSan PASS (LSan off): bitrate
wybranego wariantu, poprawna/błędna średnia/overflow, osobne audio rendition
bez obcego bitrate, zachowanie przez dwa reload/redirect i ignorowanie
sprzecznego icy-br segmentu, zachowanie icy-br gdy brak master deklaracji.
Istniejące tests actor/UI publikacji/czyszczenia pola nadal PASS. Native
ESP-IDF6.1 build PASS0x17ba90,90% app wolne, GPIO/diff PASS. Nie jest to
odbiór na S3. Zwykły flash bez erase/importu; sprawdzić OLED po starcie i
kilku reload: MP3/AAC128k i HE-AAC64k. Nie wykonano flash/erase/eFuse.
Stabilność HLS/FLAC i wcześniejsze odbiory pozostają osobnymi zadaniami.
