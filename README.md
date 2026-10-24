# StreamOO — ESP32-S3 Internet Radio

**Status:** repozytorium startowe + specyfikacja dla Codex; **nie jest to gotowe radio**.

Celem jest nowa implementacja natywna ESP-IDF, nie port starego programu Arduino. Materiałem odniesienia przy rekonstrukcji zachowania jest yoRadio; implementacja musi być niezależna i spełniać wymagania opisane w `docs/`.

## Uruchomienie prac
1. Czytaj `CODEX_MASTER_PROMPT.md`, `AGENTS.md`, `docs/00_INDEX.md`.
2. Wybierz ESP-IDF (wersję zweryfikować i przypiąć przed rozpoczęciem implementacji), ustaw środowisko, następnie `idf.py set-target esp32s3`.
3. Sprawdź `idf.py menuconfig` → `StreamOO Configuration`. Profil domyślny odpowiada schematowi YR PCB 0.3.1.
4. Realizuj po kolei etapy `docs/01_*` … `docs/12_*`, sprawdzając testy i odnotowując postęp w `docs/STATUS.md`.

**Uwaga:** do projektu nie dołączono bibliotek zewnętrznych, czcionek TTF ani źródeł yoRadio. Przygotowano katalog `tools/`, dokumentację i działający minimalny punkt wejścia ESP-IDF (jeśli dostępna jest zgodna wersja toolchain). Funkcje radia zostaną dodane etapowo.

## Decyzje
- ESP32-S3-WROOM-1-N16R8, ESP-IDF bez Arduino, PCM5102A, SSD1322 256×64 4bpp, jeden enkoder.
- Renderer własny; Monaspace Krypton dla dużych tekstów, bitmapa 5×7/wiersz 8px z polskimi znakami dla mniejszych.
- Tryby wizualizacji: spectrum FFT/Hann, L/R bargraph, 2 mierniki wskazówkowe; gest przytrzymanie + obrót.
- Import/eksport list stacji zgodny z **formatem yoRadio**; REST API i interfejs WWW nowe.
- Po 5 sekundach bez zmian zapis ustawień do NVS.

Dokumenty zawierają statusy **USTALONE / PROPONOWANE / DO POTWIERDZENIA**. Nie traktować zaproponowanych wartości jako ustaleń użytkownika.
