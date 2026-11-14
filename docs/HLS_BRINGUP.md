# Odbiór HLS — ADR0060

Po zwykłym flash bez erase/obrazu FS i Ctrl+F5 sprawdź najpierw gesty i
ustawienia z PLAYER_SETTINGS_BRINGUP. Nie trzeba ponownie importować
istniejącego katalogu. Firmware czyta dotychczasowy format.

Publiczne próby: `playlist.hls.csv` w katalogu projektu,6 adresów RadioMast
MP3 stereo/mono, AAC-LC, HE-AACv1/v2. Manifesty i próbki pobrane2026-10-31,
sprawdzone własnym parserem oraz ffprobe na komputerze; to nie odbiór S3.
Źródło: https://www.radiomast.io/reference-streams oraz
https://www.radiomast.io/docs/streaming-network/hls.html .
Raport `playlist.hls.report.json` zawiera faktyczny profil próbki, który
może się różnić od nominalnej nazwy stacji. Pliki ignorowaneGit; prywatnego
playlist.csv nie zmieniono. Bieżący import WWW dopisuje stacje (ADR0070).
Wyeksportuj listę przed próbą, aby mieć kopię; możesz też dodać wybrane
adresy ręcznie. Instrukcja: [USER_GUIDE.md](USER_GUIDE.md#import-pliku).

1. Zacznij od HLS MP3 stereo i AAC-LC, potem HE-AAC/v2 i mono. Słuchaj
   co najmniej3minut: po12s granicach segmentów i kolejnych reloadach bez
   powtarzania/przerw. Kontroluj phase/compressed/PCM/invalid/low/hard,
   heap/min/largest i reader/player HWM. Latencja live zależy od target
   serwera; start od ostatnich3 segmentów nie jest trybem low latency.
2. STOP długim kliknięciem wygasza OLED; PLAY wybudza i zaczyna nową sesję.
   Sprawdź szybkie STOP/zmiana stacji oraz odłączenie sieci podczas HLS.
   Kolejny wybór tej samej grającej pozycji nie restartuje sesji.
3. Lokalny MPEG-TS i packed AAC: wygeneruj nowy katalog poleceniem
   `python3 tools/generate_extended_fixtures.py --output-dir /tmp/streamoo-hls-lan --base-url http://ADRES_KOMPUTERA:8000`.
   Uruchom wskazany przez generator serwer HTTP, dodaj wpisy HLS z jego
   playlist.csv lub zaimportuj po eksporcie własnej listy. Oryginalne
   sygnały32s L1k/R2k z przerwami; po ENDLIST EOF/retry jest oczekiwane.
   Na OLED TS opisany MPEG-TS; rzeczywisty odsłuch parsera SDK nadal OPEN.
4. Zaszyfrowane/fMP4/BYTERANGE/LL-HLS i discontinuity po starcie kończą się
   jawnym błędem/retry. Brak insecure TLS lub automatycznego HTTP fallback.

Testy hostowe/ASan/UBSan i build PASS. Realny multicast/NVS/OLED/DMA,
wydajność TS/HE-AAC, TLS S3, stabilność HLS po wielogodzinnym odtwarzaniu
oraz stare poprawki live FLAC/RNŚ pozostają osobnymi punktami OPEN.

## Poprawka odświeżania sesji — ADR0065

Pierwszy odsłuch S3 HLS MP3 był poprawny, po~37s HTTP200/NOT_SUPPORTED.
GetterURL w pinned SDK usuwał query/lsid; reload trafiał na MASTER zamiast
MEDIA. Reader przechowuje teraz pełne adresy, także po redirect query-only
oraz segmentach ze znakowanym URL. Host odtwarza błąd starego wariantu i
poprawne wielokrotne reload nowego. Po zwykłym flash bez erase/importu
sprawdzić TEST HLS128k-mp3-stereo>=3min, gen/starts bez restartów, low0.
Odbiór poprawki S3 OPEN; ograniczenia AES/fMP4/discontinuity pozostają.

ADR0066: zgłoszony HE-AACv2 HLS ma podobny wzorzec HTTP200/NOT_SUPPORTED po
trzech segmentach. Nowe logi mają nazwę stacji/ID i wersję firmware; przy
awarii Reader pokaże dokładny resource/stage/reason, sequence/reload/segments
oraz errno/TLS. Przy source_eof zachowuje pierwotny błąd HLS zamiast go
maskować. Po nowym flash zbierać komplet z tą samą generation (instrukcja
HTTP_PLAYER_BRINGUP). Odbiór queryfixu na S3 i FLAC wciąż osobno OPEN.

ADR0067: wskaźnik bitrate odczytuje wybrany AVERAGE-BANDWIDTH/BANDWIDTH
z mastera, po konwersji do kb/s, np.128k/64k. Wartość zachowana po reload
oraz redirect/segmentach; jest deklaracją nadawcy, nie pomiarem VBR. Brak
atrybutów lub osobny audio rendition bez bitrate i bez icy-br nadal daje --.
Po zwykłym flash sprawdzić OLED po kilku segmentach; odbiór wizualny OPEN.
