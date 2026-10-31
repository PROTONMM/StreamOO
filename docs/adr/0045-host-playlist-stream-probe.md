# ADR0045 — narzędzie hostowe do sprawdzania stacji

Data: 2026-10-31. Status: wdrożone na polecenie użytkownika.

Po odbiorze MP3 na OLED użytkownik chce znaleźć AAC i odrzucić nieaktywne
stacje z playlisty. Dodajemy własny Python tools/probe_playlist.py, bez
zmian firmware i bez uruchamiania prywatnej listy przez agenta.

Input jak sprawdzony importer:3 literalne pola TAB UTF-8, name/URL/ovol.
Wspólny walidator check_playlist, jawna normalizacja U+200B w kopii.
Źródło pozostaje nietknięte. Nowe wyjścia tworzone exclusive, oryginału
nie można wskazać jako output/report. Kolejność/duplikaty/ovol zachowane;
URL próbkowane raz mimo duplikatów. Wyniki prywatne wykluczone z Git.

Zewnętrzny ffprobe zbiera metadata i do200 pakietów audio z ograniczeniem
czasu, retries i równoległości. Bitrate z wielkości/czasu pakietów, fallback
stream.bit_rate lub nieznany. Profil rozpoznany przez ffprobe w raporcie.
ACTIVE wymaga audio i pakietów, nie samego połączenia HTTP. Domyślnie
kopia zawiera tylko ACTIVE; keep-uncertain zachowuje timeout/błędy
niejednoznaczne. 404/410/NO_AUDIO wykluczone także w tym trybie.

Audyt: oficjalne https://ffmpeg.org/ffprobe.html, lokalne ffprobe6.1.1
wersja/API/opcje oraz `ffprobe -L`: GPLv2-or-later dla lokalnego buildu.
To osobny zainstalowany program hosta, bez importu/linkowania GPL do
firmware i bez dołączania binarki/źródeł FFmpeg do repo. Python stdlib,
brak nowych pakietów pip lub komponentów ESP-IDF. Nie kopiowano kodu
stron trzecich. TLS weryfikowany; whitelista protokołów sieciowych bez file.

314 testów hostowych PASS.7 nowych testów: bitrate/profile/fallback,
HTTP/timeout/JSON, retries, parser/normalizacja, oryginał/order/duplikaty,
exclusive output i rzeczywista CLI z syntetycznym ffprobe. Lokalny HTTP
127.0.0.1 z prawdziwym ffprobe rozpoznał MP3 128kb/s i AAC-LC około73.5kb/s
z istniejących klipów oraz404. Wymagało rozszerzenia sandbox o lokalny
socket; brak połączeń do prywatnych stacji.

Native ESP-IDF6.1 incremental build PASS, firmware bez zmian.
Pełny audyt dostępności267 prywatnych stacji pozostaje do uruchomienia
przez użytkownika; nie deklarujemy wyników bez połączeń. PLAYLIST_PROBE.md.
