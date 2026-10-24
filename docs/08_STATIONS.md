# 08 — Stacje, ICY, import/eksport

**USTALONE:** jedna lista na początek; kompatybilność importu/eksportu z rzeczywistym formatem pliku stacji **yoRadio**, bez kopiowania starego HTTP API. Przechowywanie LittleFS, UI/WWW edytuje listę bez restartu.

## Model
`station { stable_id, display_name, url, enabled, sort_order }`, dodatkowe metadane stanu strumienia (ICY/title/artist/codec/bitrate) poza trwałą listą. Stabilny ID niezależny od indeksu i przestawiania listy; NVS zapamiętuje stable_id ostatniej stacji.

## Import
- Upload→ parser zgodny ze **sprawdzonym na rzeczywistych plikach** formatem yoRadio→ walidacja pól/UTF-8/długości/url/duplikatów→ podgląd→ commit atomowy.
- Tryby: replace lub merge (proponowane); duplikaty raportować, bez cichego nadpisania.
- Zapis przez staging + walidacja CRC/checksum + atomowe przełączenie pliku/manifestu; przerwany import pozostawia poprzednią listę.
- Eksport zgodny z tym samym formatem (weryfikować testem round-trip). **OTWARTE:** dokładna gramatyka importu; plik źródłowy musi zostać dostarczony/wyodrębniony z archiwum yoRadio, a parser dopiero potem zaimplementowany.

## Zarządzanie
- WWW: dodaj/usuń/edytuj nazwę+URL/zmień kolejność, import/export.
- Gdy zmienia się kolejność aktualnie grającej stacji, audio trwa. Usunięcie aktywnej stacji: stop i komunikat, nie wybieraj losowo następnej (propozycja do testu).
- Zmiana URL aktywnej stacji: reconnect dopiero po zatwierdzeniu aktualizacji.
- Metadata ICY może zawierać tytuł i wykonawcę; usuwać pola przy zmianie stacji, nie utrzymywać pozostałości starej.

## Testy
Importer na prawdziwych plikach, round-trip import/export, polskie znaki, długie URL, duplikaty, przerwany zapis, zanik zasilania, zmiana kolejności podczas PLAYING i szybki wybór stacji.
