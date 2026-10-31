# ADR0066 — stacja w logu i pierwotna przyczyna błędu czytnika

Data:2026-10-31. Status: implementacja/testy/build PASS; odbiór logów S3 OPEN.

Użytkownik podał TEST HLS64k-heaacv2-stereo i wymaga nazwy stacji przy
zmianie oraz dokładniejszych błędów. Log bf6f3367: low0/hard0/starts1,
compressed maleje po około40KiB/5s, po295413B stage=http_end/HTTP200/
NOT_SUPPORTED, PCM44100/stereo16. Wzorzec pasuje do trzech początkowych
segmentów i ADR0065. Brak Decode perf nie potwierdza wgrania ostatnich
zmian; nie uznajemy tego za nowy udowodniony błąd binarnego HE-AACv2.
Zmiana dotyczy identyfikacji i diagnostyki, nie kolejnej regulacji buforów.

## Identyfikacja

- Boot: firmware version i IDF z esp_app_get_description oraz pierwsze4B
  ELF SHA256, diagnostics=2. To rzeczywiste metadane obrazu, nie ręczny hash
  commita ani metryki AI. API/komponent esp_app_format sprawdzone w pinned6.1.
- Owner po zmianie aktywnego ID: Station changed, generation, poprzedni ID,
  nowy16hex ID, name i running. Obejmuje przywrócenie po boocie i usunięcie.
  Szybkie komendy mogą zostać scalone do ostatniej faktycznej zmiany ownera.
- Każde Connect/Connect failed oraz Stream failed zawiera ID i nazwę;
  retry ma nową generation i tę samą nazwę/ID. Ten sam grający ID jest nadal
  no-op bez dodatkowego eventu, restartu bufora i dekodera.
- Nazwa z katalogu, nie z URL: do128B, UTF8/PL zachowane, kontrolne ASCII
  zamienione na?, bez rozcinania końcowego UTF8 przy ograniczeniu. Dane
  przekazywane przez%s, nie jako format printf. Bez nowych logów URL/query.

## Pierwotny błąd transportu

Snapshot HTTP otrzymuje bounded resource/stage/reason, MIME z usuniętymi
kontrolnymi znakami, ostatni wynik read, bajty odpowiedzi/sesji i read_calls,
redirect count oraz HLS: bieżąca/przyszła sequence, media sequence/count,
liczba segmentów i reload. Są to dane ownera czytnika, kopiowane pod lock;
bez wskaźników do zwalnianego kontekstu i bez nazw/adresów sieci.

Resource rozróżnia stream/playlist/hls_manifest/hls_segment/hls_reload;
stage: client_init/set_url/open/headers/read/sniff/parse/sequence/reload_wait.
Reason jest zatrzaskiwane przy pierwszej terminalnej decyzji. Przykłady:
http_open, fetch_headers, http_status, content_type, invalid_icy_interval,
redirect_location/redirect_url_rejected/redirect_limit, http_read/read_timeout/
http_eof, document_limit, manifest_invalid/manifest_feature/master_after_media/
master_limit/media_window_missed/segment_sequence/discontinuity/manifest_stalled,
segment_limit/id3_header/id3_limit/codec_probe/codec_change, sequence_overflow,
endlist i cancelled. Nie zmienia to ograniczeń HLS z ADR0060 ani ADR0065.

Przed cleanup klienta pobierane esp_http_client_get_errno i, dlaHTTPS,
esp_http_client_get_and_clear_last_tls_error: wynik ESP, szczegółowy kod
mbedTLS i flags. API/ograniczenia sprawdzone w dokładnym SDK6.1. Gettery
czyszczą ostatni dostępny błąd; brak dostępnego errno bywa-1. Pola nie
stanowią osobnego dowodu poprawności certyfikatu ani pomiaru czasu CPU.
HTTP status/MIME/response_bytes/last_read zerowane dla każdej odpowiedzi,
żeby nie pokazać200 z poprzedniego segmentu po nieudanym kolejnym open.

Czytnik publikuje końcowy snapshot przed udostępnieniem EOF bufora; kolejka
pozostaje drainable. Player ponownie odczytuje snapshot przy awarii przed
logowaniem. Dzięki temu source_eof nie maskuje np. master_after_media lub
błędu TCP/TLS. Bez zmiany routingu EOF/retry i bez odrzucania gotowych bajtów.
Nowa linia Reader dołączana przy błędzie, nie co5s; istniejące Decode perf
z ADR0064 pozostaje. Decoder ESP error ma teraz nazwę i wartość, vendor rawcode
zachowane. HWM readera mierzone także podczas HLS, nie pozostaje sztuczne0.

## Granice i weryfikacja

Wybór logowany po uruchomieniu fade, jeżeli zamykana jest poprzednia sesja;
rozbudowane błędy również po target_gain0. Nie dodajemy długich okresowych
linii do ścieżki aktywnego zasilania PCM. Counters i małe bounded copy w tasku
HTTP/player; bez print/alloc/flash/FFT/locków HTTP w callbacku I2S. Dodatkowe
pola diagnostyczne w istniejącym obiekcie PSRAM i snapshotach tasku. Brak
nowych zewnętrznych zależności i kodu, GPIO/partycje/NVS/WWW/logo bez zmian.

377 host tests PASS, HTTP/player ASanUBSan PASS (LSan off). Testy: nazwa PL,
neutralizacjaESC, zmiany i no-op tego samego ID, nazwa na błędzie, oryginalny
HLS error po źródłowym EOF powstałym po pierwszym snapshotcie, errno/TLS
przed cleanup, HLS sequence/reload/segment counters i nazwane powody. Błąd
set_url z SDK pozostaje oryginalnym kodem również w M3U i HLS (test NO_MEM).
AP startup z rzeczywistym main.c nadal PASS. Native ESP-IDF6.1 build PASS
0x17b990,90% app wolne, GPIO/diff PASS. Nie jest to odsłuch/handshake S3.

Po zwykłym flash bez erase/FS/importu: zachować linię firmware z boot,
Station changed/Connect oraz Stream failed/Reader/Decode perf/Decode diag
z tej samej generation. Wgrać aktualny firmware obejmujący ADR0064/0065;
sprawdzić HE-AACv2 HLS przez wiele reload. Odsłuch HLS i FLAC nadal OPEN.
Agent nie wykonywał flash/erase/eFuse. Sekrety AP/STA i prywatny playlist.csv
nietknięte. Nie diagnozować samego profilu HE-AAC z nazwy stacji ani rate.
