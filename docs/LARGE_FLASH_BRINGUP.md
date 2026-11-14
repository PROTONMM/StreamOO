# Duża aplikacja bez OTA i ponowny import stacji

Bieżący układ: factory0x10000/0xEF0000 (14,9375MiB), LittleFS0xF00000/1MiB.
NVS/PHY bez zmian. Bez OTA. Nie używaj erase-flash; ustawienia mają pozostać.

1. Wgraj cały bieżący build: `idf.py -p PORT flash` — obejmuje nową tablicę
   partycji i aplikację. Samo app-flash ze starą tablicą nie przeniesie storage.
2. Po starcie sprawdź połączenie z zapisaną Wi-Fi. Nowy pusty LittleFS
   zostanie przygotowany jednorazowo; katalog będzie pusty.
3. Otwórz WWW po IP lub dotychczasowej nazwie .local. Zaimportuj przygotowany
   plik `playlist.codecs.csv` z głównego katalogu projektu. Do własnej listy
   wrócisz później przez import swojego `playlist.csv`, którego nie zmieniano.

## Lista kodeków

13 pozycji, wszystkie sprawdzone na komputerze2026-10-31:

| Pozycje | Test |
|---|---|
| Radio Mast MP3 stereo i mono | punkt odniesienia i kanały |
| Radio Mast AAC-LC | regresja AAC |
| Radio Mast HE-AAC v1 oraz v2 | SBR, SBR+PS |
| Radio Mast Ogg Vorbis | parser Ogg + Vorbis |
| Radio Mast Ogg Opus | parser Ogg + Opus |
| Radio Mast FLAC native | strumień FLAC, bez Ogg |
| Kiss FM, RNŚ AAC+, Radio357 | rzeczywiste polskie stacje AAC/HE-AAC |
| Radio Mast M3U | rozwiązanie playlisty do MP3 |
| RNŚ PLS | rozwiązanie File1 do MP3 |

`playlist.codecs.details.csv` zawiera wykryte kodeki/profil/bitrate próbek,
częstotliwość/kanały i URL po rozwiązaniu playlisty. `playlist.codecs.report.json`
ma szczegóły weryfikacji. Żaden wynik nie jest deklaracją odsłuchu na ESP32.
Otwieraj kolejno, sprawdzaj poprawne tempo, wysokość dźwięku, kanały,
PLAY/STOP, przełączanie stacji oraz heap/stack_hwm/underrun. Podeślij log
przy błędzie, podając nazwę testu. Wykresy FFT mogą być używane normalnie.

Źródła adresów: [Radio Mast reference streams](https://www.radiomast.io/reference-streams),
[RNŚ — jak słuchać](https://nowyswiat.online/jak-nas-sluchac) i publiczny Radio Browser.
Adresy/profil mogą się zmienić. HLS i Ogg-FLAC pozostają poza zakresem tej listy.

## Jeżeli nowy LittleFS nie daje się przygotować

Firmware nie formatuje automatycznie niepustego, nierozpoznanego storage.
Jeśli jego obszar był zajęty przez wcześniejszy firmware, po sprawdzeniu logu
można JEDNORAZOWO wyczyścić tylko nową partycję stacji:

```sh
python -m esptool --chip esp32s3 --port PORT erase-region 0xF00000 0x100000
```

Polecenie wykona użytkownik przy zamkniętym monitorze UART; potem restart
radia i ponowny import. Dotyczy wyłącznie aktualnego układu z ADR0055.
Nie dotyka NVS/PHY/aplikacji i nie jest wymagane, gdy storage jest pusty.
Agent nie wykonuje tego polecenia na płytce.
