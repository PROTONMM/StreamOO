# ADR0035 — powód błędu live decoder zamiast domysłu z formatu

Data: 2026-10-31. Status: build/testy PASS; identyfikacja przyczyny retry z
odbioru06i nadal OPEN do następnego zdarzenia na sprzęcie.

## Problem i decyzja

Odsłuch kilku stacji zaliczony. Jeden wpis gen7 z 740c327 pokazał HTTP200,
decoder0/vendor0 i format22050/2/16, po czym retry wrócił do PLAYING44100.
Ostatnio zaobserwowany format nie jest dowodem przyczyny: 22,05kHz mieści się
w obsługiwanym zakresie. Nie zmieniamy resamplera ani obsługi formatów na
podstawie tej linii. Najpierw rozdzielamy istniejące warunki awarii.

Live pump zatrzaskuje pierwszy terminalny powód na sesję:

| cause | Warunek |
|---|---|
| source_eof | Koniec źródła, bez rewind/odtwarzania od początku |
| source_error | Callback źródła zwrócił ERROR |
| source_contract | DATA bez danych/ponad bufor albo n/last przy non-DATA |
| decoder_error | API dekodera zwróciło błąd |
| decoder_bounds | Wynik consumed/produced przekracza przekazane bufory |
| decoder_no_progress | API zwróciło OK, ale consumed=produced=0 |
| pcm_format | Nieobsługiwane bits/channels/rate lub niepełna ramka PCM |
| format_changed | Obsługiwany format różni się od wcześniejszego w sesji |

Normalny WAIT i wyczerpanie budżetu pracy nie są błędami. Dodatkowe sprawdzenie
kontraktu non-DATA odrzuca niepoprawny callback; poprawny buffered_source
spełnia ten warunek. Kolejny read po awarii nie nadpisuje przyczyny/liczników,
a init nowej sesji je zeruje. Polityka reconnect i guard zmiany formatu pozostają.

## Dane diagnostyczne

`Stream failed`: stage (http_end/decoder_open/memory/pipeline_init/i2s_open/
decode/pcm_queue/i2s_stats), cause pumpa, HTTP/error/decoder/vendor,
`previous` (ostatni zaakceptowany format), `format` (ostatni wyprodukowany).
Błąd startu readera osobno: `Connect failed ... stage=http_open`.
Brak pumpa albo błąd innego modułu może dawać cause=none — wtedy istotny stage.

`Decode diag` z tą samą gen: in/used/out/needed ostatniego wywołania,
calls/reads, fetched/consumed/produced łącznie, pending i partial (złożone
ramki wyjściowego bloku). Sumy consumed/produced obejmują wyniki OK mieszczące się w buforach;
used/out ostatniej próby pokazują też błędny wynik. Produced nie oznacza
próbek odtworzonych przez I2S. Pola numeryczne, bez URL, treści
playlisty lub audio. Dwie linie emitowane tylko przy awarii.

Komenda gain=0 jest wydawana przed logowaniem. Okno100ms na fade/DMA liczone
od aktualnego czasu, żeby druk UART nie opóźniał samego rozpoczęcia fade.
Formatowanie/logowanie i liczniki tylko w tasku; brak zmian callbacka I2S,
nowych alokacji, zależności lub bibliotek. Snapshot pumpa rośnie o stałe pola.

## Weryfikacja

248 testów hostowych PASS. Nowy test obejmuje15 wektorów wszystkich klas
awarii, w tym poprawne22050 poprzedzające no-progress oraz44100→22050 jako
format_changed; różne previous/observed, status/needed, liczniki, EOF, brak
zapisu do wyjścia przy failure i reset/latch. Actor test sprawdza rzeczywiste
formatowanie obu linii z cause=decoder_no_progress i danymi ostatniej próby.
Poprzednie testy próbek 48k/44,1k/mono22,05k i cyklu playera pozostają PASS.
Targeted live/actor ASan/UBSan PASS, LSan off, bez TSan. Native ESP-IDF6.1
build PASS, diff --check i check_config --platform PASS. Bez flash/erase/eFuse.

Przy następnym retry zebrać `Stream failed` i sąsiednie `Decode diag` dla
tej samej gen. Dopiero one pozwolą wybrać naprawę. Nie ogłaszać wyjaśnienia
lub usunięcia pierwotnej przyczyny na podstawie samych testów diagnostyki.
