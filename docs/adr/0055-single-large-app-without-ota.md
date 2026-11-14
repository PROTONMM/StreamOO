# ADR0055 — jedna duża aplikacja, bez OTA

Data:2026-10-31. Status: wdrożone; flash/nowy filesystem na płytce OPEN.

Użytkownik rezygnuje z OTA, zleca jedną dużą partycję aplikacji i zgadza
się ponownie wgrać stacje. To zastępuje wcześniejsze wymagania2×OTA i
zachowania offsetu filesystemu. Ustawienia NVS zachowujemy.

## Układ16MiB

| Partycja | Offset | Rozmiar | Znaczenie |
|---|---|---|---|
| nvs | 0x9000 | 0x6000 | bez zmian: sieci, głośność, widget, strefa |
| phy_init | 0xF000 | 0x1000 | bez zmian |
| factory | 0x10000 | 0xEF0000 | jedna aplikacja,14,9375MiB |
| storage | 0xF00000 | 0x100000 | LittleFS1MiB na końcu flash |

Bez ota_0/ota_1/otadata i bez rezerwacji OTA. Standardowy przewodowy flash,
bez erase-flash. Tablica i aplikacja muszą pochodzić z bieżącego buildu.
Stare LittleFS0x190000 jest porzucone, nie migrujemy katalogu. Zgodnie
z decyzją użytkownika stacje trzeba zaimportować ponownie. Prywatny
playlist.csv na komputerze pozostaje nietknięty. Nie wykonano flash,
erase-region/erase-flash ani eFuse przez agenta.

## Pierwsze uruchomienie i NVS

Dotychczasowy globalny streamoo_fs/init mógłby blokować format nowego,
pustego filesystemu, bo zachowujemy NVS. Marker fazy inicjalizacji jest
teraz przypisany do adresu partycji: sfs_00f00000/init. Stary marker
pozostaje niewykorzystywany; profile i pozostałe namespace nie są kasowane.

Reguły ochrony istniejących danych pozostają: mount bez autoformatowania;
pierwszy format tylko gdy cała nowa partycja jest0xFF, albo marker tej
samej partycji wskazuje rozpoczętą inicjalizację. Potem phase2. Uszkodzony,
niepusty filesystem nie jest automatycznie kasowany. W razie zajętego
końca flash procedura dokumentuje ręczne, jednorazowe czyszczenie WYŁĄCZNIE
nowego storage; nie kasować NVS ani całego układu.

Testy adaptera NVS zawierają stary marker phase2 i nowy brak markera,
pierwszy format blank, recovery phase1 oraz odmowę formatu nonblank/phase2
lub błędu commit. Test układu sprawdza brak OTA, jeden factory>14MiB,
NVS/PHYoffsety, wyrównanie, brak overlap i koniec dokładnie16MiB.

## Lista do odbioru nowych kodeków

Na żądanie użytkownika przygotowano lokalny playlist.codecs.csv,13 pozycji
oraz CSV szczegółów i JSON raportu. To publiczne linki, nie prywatny katalog;
pliki pozostają poza Git jak poprzednie lokalne wyniki playlist.

Oficjalne Radio Mast Reference Streams dają MP3 stereo/mono, AAC-LC,
HE-AACv1/v2, Ogg Vorbis, Ogg Opus i raw/native FLAC. Dodano polskie KissFM
HE-AAC, RNŚAAC+, Radio357AAC-LC; M3U RadioMast i PLS RNŚ. HTTPS zweryfikowane
przez domyślny truststore Python,64KiB audio/ffprobe. M3U/PLS rozwiązane
rzeczywistym czystym adapterem StreamOO, a ich cel sprawdzony audio.
SomaFM PLS miało poprawny dokument, ale cel zwrócił403 — nie trafił do listy.
Nominalny HE-AAC24k mono RadioMast został pominięty, bo próbka zgłosiła
HE-AACv2/stereo zamiast opisu nadawcy. Ogg-FLAC/HLS nie dodano jako testów
pozytywnych, bo obecny firmware ich nie deklaruje.

Źródła:
- https://www.radiomast.io/reference-streams
- https://nowyswiat.online/jak-nas-sluchac
- https://www.radio-browser.info/

Weryfikacja hosta nie potwierdza dekodowania na S3. Procedura LARGE_FLASH_BRINGUP.md.
