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
playlist.csv nie zmieniono. Import pliku w WWW zastępuje listę, więc eksportuj
ją przed próbą, albo dodaj wybrane adresy ręcznie w edytorze.

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
