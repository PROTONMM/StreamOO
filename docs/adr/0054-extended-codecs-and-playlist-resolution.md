# ADR0054 — rozszerzone kodeki i adresy M3U/PLS

Data:2026-10-31. Status: implementacja i host/native build; odbiór S3 OPEN.

Użytkownik zlecił HE-AAC/AAC+, FLAC, Ogg Vorbis, Opus, M3U/PLS i ewentualnie
HLS. W tym kroku podłączamy cztery rodziny dekoderów i rozwiązywanie adresów
playlist. HLS jest osobnym transportem segmentowym i pozostaje OPEN; nie
traktujemy go jako zwykłej M3U i nie wysyłamy segmentów TS do dekodera MP3.

## Dekodery i PCM

Bez zmiany wersji: espressif/esp_audio_codec2.6.2, zgodny z przypiętym
IDF6.1/ESP32-S3. Audyt publicznych API/ograniczeń i LICENSE oraz vendor
COPYRIGHT wykonany przed integracją. Espressif Modified MIT, MP3/AAC
Apache2, Xiph FLAC/Vorbis/Opus BSD. Retained notices w licenses i
THIRD_PARTY_NOTICES. Nie używamy Arduino/ADF ani kodu yoRadio/GPL.

- AAC w ADTS: `aac_plus_enable=true` dla LC, HE-AAC i HE-AACv2. Historyczna
  nazwa enum AAC_LC zachowana dla istniejącego fixture; UI nadal ogólne AAC,
  nie zgaduje profilu z MIME.
- Native FLAC przez Simple Decoder; MIME audio/flac i audio/x-flac lub
  sygnatura fLaC. Nie deklarujemy Ogg-FLAC.
- Vorbis i Opus przez parser/dekoder kontenera Ogg, z jawną rejestracją
  tylko wybranego backendu oraz Ogg. Rozpoznanie pierwszej strony BOS
  OggS/OpusHead/identification Vorbis działa także przy fragmentach1B.
  Sprawdzenie właściwego kontenera/dekodowanie pozostaje w bibliotece SDK.
  Nie deklarujemy surowego Opus, RTP ani Ogg wielokanałowego/chained.

Live PCM: mono/stereo,8..48kHz,16/24/32-bit little-endian. Rozszerzony
bezpieczny odczyt signed PCM do Q31, interpolacja z int64 bez overflow
różnicy skrajnych próbek. Zachowane stereo48k/32 na I2S, gain/rampa/metry
przed gain. Zmiana formatu w trakcie sesji nadal kończy ją błędem.

Scratch dekodera64KiB (poprzednio8KiB) i cały pump teraz w PSRAM; obsługuje
m.in. Opus120ms stereo oraz typowe ramki FLAC. Większa wymagana ramka
jest odrzucana z diagnostyką needed_size, bez nieograniczonej alokacji.
To limit implementacji, nie pełen zakres specyfikacji FLAC. Format>48kHz
lub>2 kanały jest odrzucany; SRC wysokich częstotliwości osobno OPEN.
Player stack24KiB zamiast12KiB: producent zaleca około20KiB dla wszystkich
dekoderów, z zapasem na nasz caller. Rzeczywiste HWM/CPU/heap/underrun
należy zmierzyć na S3. W callbacku I2S bez nowych operacji/alokacji.

## Rozwiązywanie playlist

Osobny czysty adapter streamoo_playlist_resolver, bez sieci/storage/audio.
Reader HTTP rozpoznaje typy M3U/PLS/text/plain lub rozszerzenie odpowiedzi
przy nieznanym kodeku. Pobiera dokument maks16KiB, z terminatorem w zapasie
istniejącego bufora PSRAM przed zapisaniem pierwszych bajtów audio.
BOM, CR/LF/CRLF, komentarze, EXTINF, względne URI/dot segments i schemeless
//host są obsługiwane. M3U wybiera pierwszą pozycję, PLS najniższe FileN
(1..64), niezależnie od kolejności wierszy. Nie przeskakuje na inną stację
po błędzie wybranego strumienia.

Do3 rozwiązań playlist i5 przekierowań HTTP na sesję; pętle/za duże dane/
NUL w dokumencie/błędne URI kończą próbę. Timeout i cancel readera nadal
obowiązują, długi odczyt dokumentu ustępuje CPU. Adresy tylko HTTP(S),
bez credentials/fragmentów, do2048B; zakaz HTTPS→HTTP dotyczy zarówno
przekierowania, jak i zawartości playlisty. Katalog zachowuje oryginalny URL
playlisty i stable ID; URL docelowy jest tylko lokalny dla sesji.

`#EXT-X-` wykryte w dowolnym miejscu dokumentu daje NOT_SUPPORTED. Zwykła
M3U8 bez HLS może działać jako M3U. Nie deklarujemy DASH, HLS segmentów,
AES, fMP4, adaptacyjnego bitrate ani importu M3U jako katalogu stacji;
import katalogu przez WWW nadal używa CSV/TAB.

## Miejsce na flash

Nowe backends przy debug/Og przekraczają stary slot. Wybieramy -Os i
silent assertions: kontrole assert pozostają, błąd nadal abortuje z adresem/
backtrace; bez tekstu wyrażenia/numeru linii. Zachować ELF do dekodowania
backtrace. Bez -DNDEBUG i bez osłabiania walidacji aplikacji/TLS.

Factory offset0x10000 zachowany; rozmiar0x177000→0x180000 wykorzystuje
wyłącznie pustą lukę do0x190000. Storage LittleFS nadal offset0x190000,
size0x100000; NVS/PHY bez zmian. Nie ma migracji danych/formatowania/erase.
Standardowy flash powinien zawierać wygenerowaną tablicę partycji. Ten krok
nie dodaje OTA; dalsza większa rozbudowa nadal wymaga planu partycji16MiB.

## Weryfikacja i odbiór

Testy realnego adaptera z publicznymi nagłówkami dostawcy i atrapami binary
calls: konfiguracja/register/open/process/bounds/cleanup,35 kombinacji.
Realny reader z fake transport: sygnatury FLAC/Ogg, fragmenty1B, przejście
M3U/PLS→audio, nested limit, HLS, limit dokumentu, downgrade, cancel i
regresje ICY/TLS. Pure resolver: URI/PLS/BOM/limity i3000 wejść corpus.
PCM: skrajne wartości16/24/32, ciągłość across WAIT/packets, resampling16.
To nie jest wykonanie binarnych dekoderów na hoście.

Generator tools/generate_extended_fixtures.py tworzy oryginalne32s sygnały
MP3/AAC-LC/FLAC16/24/Vorbis/Opus20/120ms oraz względne/nested playlisty.
Wygenerowane lokalnie i sprawdzone ffprobe. Nie wbudowujemy ich do obrazu.
HE-AAC/v2 wymaga realnego źródła lub zweryfikowanego fixture z encoderem,
którego bieżący host nie dostarcza. Procedura EXTENDED_FORMATS_BRINGUP.md.
