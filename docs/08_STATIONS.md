# 08 — Stacje, ICY, import/eksport

**USTALONE:** jedna lista na początek; kompatybilność importu/eksportu z rzeczywistym formatem pliku stacji **yoRadio**, bez kopiowania starego HTTP API. Przechowywanie LittleFS, UI/WWW edytuje listę bez restartu.

## Model
`station { stable_id, display_name, url, enabled, sort_order }`, dodatkowe metadane stanu strumienia (ICY/title/artist/codec/bitrate) poza trwałą listą. Stabilny ID niezależny od indeksu i przestawiania listy; NVS zapamiętuje stable_id ostatniej stacji.

## Import
- Upload→ parser zgodny ze **sprawdzonym na rzeczywistych plikach** formatem yoRadio→ walidacja pól/UTF-8/długości/url/duplikatów→ podgląd→ commit atomowy.
- Tryby: replace lub merge (proponowane); duplikaty raportować, bez cichego nadpisania.
- Zapis przez staging + walidacja CRC/checksum + atomowe przełączenie pliku/manifestu; przerwany import pozostawia poprzednią listę.
- Eksport zgodny z tym samym formatem (weryfikować testem round-trip). **FORMAT SPRAWDZONY2026-10-31:** lokalny playlist.csv to UTF-8 i3 literalne pola TAB: nazwa/URL/ovol, bez nagłówka. Firmware parser/preview/zapis nadal OPEN; szczegóły ADR0030.

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
