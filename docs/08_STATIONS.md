# 08 — Stacje, ICY, import/eksport

**USTALONE:** jedna lista na początek; kompatybilność importu/eksportu z rzeczywistym formatem pliku stacji **yoRadio**, bez kopiowania starego HTTP API. Przechowywanie LittleFS, UI/WWW edytuje listę bez restartu.

## Model
`station { stable_id, display_name, url, enabled, sort_order }`, dodatkowe metadane stanu strumienia (ICY/title/artist/codec/bitrate) poza trwałą listą. Stabilny ID niezależny od indeksu i przestawiania listy; NVS zapamiętuje stable_id ostatniej stacji.

## Import
- Upload→ parser zgodny ze **sprawdzonym na rzeczywistych plikach** formatem yoRadio→ walidacja pól/UTF-8/długości/url/duplikatów→ podgląd→ commit atomowy.
- Tryby: replace lub merge (proponowane); duplikaty raportować, bez cichego nadpisania.
- Zapis przez staging + walidacja CRC/checksum + atomowe przełączenie pliku/manifestu; przerwany import pozostawia poprzednią listę.
- Eksport zgodny z tym samym formatem (weryfikować testem round-trip). **FORMAT SPRAWDZONY2026-10-31:** lokalny playlist.csv to UTF-8 i3 literalne pola TAB: nazwa/URL/ovol, bez nagłówka. Parser/preview/atomowy zapis zaimplementowane (ADR0031), publikacja i lista OLED gotowe do odbioru (ADR0033).

## Zarządzanie
- WWW: dodaj/usuń/edytuj nazwę+URL/zmień kolejność, import/export.
- Gdy zmienia się kolejność aktualnie grającej stacji, audio trwa. Usunięcie aktywnej stacji: stop i komunikat, nie wybieraj losowo następnej (propozycja do testu).
- Zmiana URL aktywnej stacji: reconnect dopiero po zatwierdzeniu aktualizacji.
- Metadata ICY może zawierać tytuł i wykonawcę; usuwać pola przy zmianie stacji, nie utrzymywać pozostałości starej.

## Testy
Importer na prawdziwych plikach, round-trip import/export, polskie znaki, długie URL, duplikaty, przerwany zapis, zanik zasilania, zmiana kolejności podczas PLAYING i szybki wybór stacji.

## Prywatny plik referencyjny i audyt

Użytkownik dostarczył playlist.csv w katalogu głównym. Plik jest ignorowany
przez Git i nie może trafić do fixture/build. 267 rekordów;11 URL zawiera
U+200B,3 URL się powtarzają. Nie poprawiono oryginału. Własny audyt:

```sh
python3 tools/check_playlist.py playlist.csv
```

Raportuje numery wierszy i kody bez treści stacji/URL. Exit1 oznacza problemy
walidacji (tu11 wierszy z niewidocznym znakiem), nie błąd działania programu.
Duplikaty tylko jako ostrzeżenia. Podgląd przyszłego importu ma jasno pokazać
korekty/duplikaty przed zatwierdzeniem. Pole ovol zachowane, mapowanie na DSP
jeszcze nieustalone. Gramatyka i źródło licencyjne w ADR0030; kod upstream
nie został skopiowany. Narzędzie nie zastępuje uploadu, parsera firmware,
LittleFS, stabilnych ID ani atomowej podmiany katalogu.

## Implementacja 08a

Zapisany katalog jest źródłem listy OLED w profilu sieciowym; snapshoty PSRAM
z refcount i aktualizacja do 1 s po commit bez restartu. ID v1 niezależne od
pozycji dla unikalnych par name/URL, duplikaty zachowane. Reorder zachowuje
wybór, usunięcie czyści go bez losowego fallback. Edycja nazwy/URL/ID oraz
player HTTP i zapamiętanie ostatniego wyboru nadal OPEN. Granice ID w ADR0033.
Odsłuch nie jest częścią odbioru katalogu: wybór na HOME nie uruchamia streamu.

06i: wybór OLED wysyła ID/generację do playera HTTP/ICY, brak optymistycznego PLAY. Reorder utrzymuje sesję tej samej stacji, usunięcie zatrzymuje bez fallback. Testy hostowe PASS, odsłuch sprzętowy OPEN. ADR0034.

ADR0051: edytor WWW i eksport3pól TAB, ręczne dodawanie/usuwanie, celowe
czyszczenie do trwałego pustego katalogu. CAS generation przed zapisem;
normalny atomic pipeline walidacja/temp/fsync/rename/snapshot. Eksport
strumieniowany z utrzymanego snapshotu, bez odczytu flash. Import pustego
pliku nadal odrzucony, clear osobnym przyciskiem. ID v1 przy rename/URL
nadal się zmienia; UI uprzedza o możliwym zatrzymaniu aktywnej pozycji.

## Trwałe ID v2 — ADR0059

Edytor WWW wysyła ID razem z nazwą/URL/ovol. Rename i zmiana URL zachowują
ID oraz aktywne połączenie; nowy URL obowiązuje przy następnym połączeniu.
Nowe rekordy dostają losowy niezerowy64-bit ID z kontrolą kolizji.
LittleFS zachowuje pojedynczy atomowy plik: `SOOCAT2\n` i wiersze
`16hex<TAB>name<TAB>url<TAB>ovol`. Stary3-polowy katalog nadal czytany,
jego dotychczasowe ID zachowane przy pierwszym zapisie. Publiczny import/
eksport CSV nadal3pola; import kojarzy istniejące pary nazwa+URL, nie może
odgadnąć tożsamości po jednoczesnej zmianie obu pól. CAS generation nadal
chroni edycję. Usunięcie aktywnego ID zatrzymuje bez fallback. NVS pamięta
ostatni wybrany ID po5s bez zmian, także przy STOP. Te zasady zastępują
ograniczenie v1 opisane historycznie wyżej.
