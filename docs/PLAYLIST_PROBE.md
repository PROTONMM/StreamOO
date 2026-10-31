# Test dostępności, kodeków i bitrate playlisty

Z katalogu projektu:

```sh
python3 tools/probe_playlist.py playlist.csv
```

Wymagania: Python3.9+ i `ffprobe` z pakietu FFmpeg w PATH. Sprawdzono
Python3.12 / ffprobe6.1.1. Nie trzeba kompilować ani wgrywać firmware.

Skrypt pokazuje tabelę: status, kodek, profil (np. AAC/LC lub HE-AAC, jeżeli
rozpoznany przez ffprobe), bitrate kb/s i nazwę/wiersz stacji. Szczegółowy
raport zawiera też sample rate, kanały, kontener, liczbę prób i źródło bitrate.

Powstają dwa nowe pliki obok oryginału:
- `playlist.active.csv`: tylko stacje z potwierdzonym audio; ten sam format
  nazwa/TAB/URL/TAB/ovol, kolejność/duplikaty/ovol zachowane.
- `playlist.report.json`: wynik każdej pozycji, również odrzuconej.

Oryginał nigdy nie jest nadpisywany. Wyjścia muszą być nowe i różne od
wejścia oraz siebie. Standardowe lokalne `/playlist.*.csv` i
`/playlist.*.json` są ignorowane przez Git; nie commitować także prywatnych
wyników pod innymi nazwami. URL/nazwy prywatnej playlisty nie są fixture.

Domyślnie4 równoległe procesy ffprobe,2 próby/adres,15s limitu każdej próby
i200 pakietów na próbkę. Powtarzające się URL testowane raz, ale wszystkie
pozycje pozostają w pierwotnej kolejności. BOM/CRLF i prefiks U+200B URL
normalizowane w kopii; liczba takich poprawek jest wyświetlana. Inne błędy
formatu przerywają przed żądaniami sieciowymi.

Bitrate to średnia z rozmiarów i czasu pakietów audio, bez narzutu sieci/ICY;
VBR może zmieniać wynik. Gdy brak czasów pakietów, używany dodatni
stream.bit_rate ffprobe, opisany `bitrate_source=ffprobe_stream`. Bez
wiarygodnych danych pokazywane ?, nigdy zgadywanie z MIME/nazwy URL.

HTTP404/410, timeout, NO_AUDIO i PROBE_ERROR są rozróżnione. Niepowodzenie
po dwóch próbach oznacza nieosiągalność z tego komputera w tym momencie,
nie dowodzi trwałego wyłączenia stacji. Do zachowania wyników niepewnych:

```sh
python3 tools/probe_playlist.py playlist.csv --keep-uncertain --timeout 20 --attempts 3
```

Ta opcja zachowuje także niejednoznaczne błędy HTTP (np.503/403), ale nie
404/410/NO_AUDIO. Przy istniejących wynikach podaj nowe nazwy:

```sh
python3 tools/probe_playlist.py playlist.csv --output playlist.active.2.csv --report playlist.report.2.json
```

ACTIVE oznacza rozpoznane audio przez ffprobe na hoście. Nie oznacza
zgodności HLS/PLS/M3U/kodeka z firmware StreamOO. Skrypt nie wybiera innej
stacji, nie zmienia URL na wykryty redirect i nie usuwa pozycji z urządzenia.
Można potem przejrzeć raport i wgrać nową listę przez WWW.

Szczegóły użytych opcji: https://ffmpeg.org/ffprobe.html . FFprobe uruchamiany
jako osobny program; nie kopiujemy ani nie linkujemy kodu FFmpeg do firmware.
Testy: `python3 -m unittest discover -s tests -p test_playlist_probe.py`.
