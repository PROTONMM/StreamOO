# Kontrolowany test FLAC z komputera w LAN

Firmware b2431e5 może pozostać na płytce. Ten krok nie zmienia jego kodu.
Potrzebne hostowe ffmpeg/ffprobe i Python3; radio oraz komputer w tej samej
sieci. Nie kasować zapisanych stacji: import playlisty dopisuje pozycje.

Gotowe lokalne pliki przygotowano w build/flac-lan-20261101. Uruchom z
głównego katalogu projektu i pozostaw terminal z serwerem otwarty:

```sh
python3 -m http.server 8000 --bind 0.0.0.0 --directory build/flac-lan-20261101
```

Na stronie WWW radia zaimportuj build/flac-lan-20261101/playlist.csv.
Następnie wybieraj pojedynczo:

1. TEST LAN FLAC16 tones — około213kbps, sinusy, pierwsze3min.
2. TEST LAN FLAC24 noise — około1839kbps, szum, pierwsze3min.
3. TEST LAN FLAC24 live headerless — te same dane audio bez nagłówka pliku,
   pierwsze3min. Sprawdza dodatkowo start parsera live.

Zachowaj konsolę UART obejmującą nazwy stacji, Delivery, Underrun,
Recover/Reject, CPU i OLED UI. Jeżeli słuchanie nie jest możliwe, wystarczy
log do porównania liczników; nie oznacza to jeszcze zaliczenia odsłuchu.
Docelowo low/hard nie rosną po starcie, brak Recover/Reject, a PCM ma zapas.
Pliki mają300s; EOF/retry przy ich końcu jest oczekiwany. HTTP nie deklaruje
icy-br. Starszy firmware pokazuje bitrate nieznany; od ADR0093 po kilku
sekundach pokazuje pomiar czasem audio z tyldą (~). Manifest zawiera
bitrate plików, a Delivery osobno pokazuje dopływ body HTTP.

Gdy potrzebna jest nowa playlista dla innego adresu komputera, wygeneruj
nowy katalog, podając adres komputera widoczny dla radia, np.:

```sh
python3 tools/generate_flac_network_fixture.py \
  --output-dir build/flac-lan-new \
  --base-url http://192.168.1.20:8000 \
  --seconds 300
python3 -m http.server 8000 --bind 0.0.0.0 --directory build/flac-lan-new
```

Generator nie nadpisuje istniejącego katalogu. Media, playlistę i manifest
trzymaj pod build/. Po błędzie generacji częściowy katalog pozostaje do
sprawdzenia; kolejna próba może użyć nowej nazwy.

Interpretacja i ograniczenia: adr/0091-controlled-lan-flac-fixtures.md.
LAN pomija jednocześnie Internet, TLS i ICY; jego sukces nie wskazuje
samodzielnie, która z tych części odpowiada za problem publicznego streamu.
