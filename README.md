# StreamOO — ESP32-S3 Internet Radio

**Status:** profil sprzętowy, walidator GPIO, baza diagnostyczna ESP-IDF, test SSD1322 i niezależny canvas 4bpp; **nie jest to gotowe radio**. Wyniki i ograniczenia: `docs/STATUS.md`.

Celem jest nowa implementacja natywna ESP-IDF, nie port starego programu Arduino. Materiałem odniesienia przy rekonstrukcji zachowania jest yoRadio; implementacja musi być niezależna i spełniać wymagania opisane w `docs/`.

## Uruchomienie prac
1. Czytaj `CODEX_MASTER_PROMPT.md`, `AGENTS.md`, `docs/00_INDEX.md`.
2. Użyj ESP-IDF 6.1 (zweryfikowane v6.1) i targetu esp32s3. Istniejącego poprawnego sdkconfig nie trzeba usuwać ani ponownie wykonywać set-target.
3. Sprawdź `idf.py menuconfig` → `StreamOO Configuration`. Profil domyślny odpowiada schematowi YR PCB 0.3.1.
4. Realizuj po kolei etapy `docs/01_*` … `docs/12_*`, sprawdzając testy i odnotowując postęp w `docs/STATUS.md`.

**Uwaga:** do projektu nie dołączono bibliotek zewnętrznych, czcionek TTF ani źródeł yoRadio. Build sprawdzono z ESP-IDF v6.1. Komponenty `board_config` i `platform` kontrolują konfigurację i raportują pamięć; opcjonalny `hardware_probe` próbkuje surowe wejścia enkodera. Funkcje radia zostaną dodane etapowo.

## Decyzje
- ESP32-S3-WROOM-1-N16R8, ESP-IDF bez Arduino, PCM5102A, SSD1322 256×64 4bpp, jeden enkoder.
- Renderer własny; Monaspace Krypton dla dużych tekstów, bitmapa 5×7/wiersz 8px z polskimi znakami dla mniejszych.
- Tryby wizualizacji: spectrum FFT/Hann, L/R bargraph, 2 mierniki wskazówkowe; gest przytrzymanie + obrót.
- Import/eksport list stacji zgodny z **formatem yoRadio**; REST API i interfejs WWW nowe.
- Po 5 sekundach bez zmian zapis ustawień do NVS.

Dokumenty zawierają statusy **USTALONE / PROPONOWANE / DO POTWIERDZENIA**. Nie traktować zaproponowanych wartości jako ustaleń użytkownika.

Testy hostowe: `python3 -m unittest discover -s tests -v`. Build: `idf.py reconfigure build`. Dla modułu msalamon R5/R8 dodano test OLED (około 38 s, potem panel OFF). Odbiór resetu i obrazu: [protokół sprzętowy](docs/HARDWARE_BRINGUP.md).

Podstawowe działanie OLED potwierdzone przez użytkownika; nowa scena renderera i szczegółowe pomiary wymagają odbioru sprzętowego. Dołączono mały font PL i jego test; większy font na bazie Krypton i marquee są w teście diagnostycznym; pełny UI nadal w implementacji. Stan licencji małego fontu: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

Model i tokeny nowych commitów: [procedura i dostępność danych](docs/COMMIT_METRICS.md). Źródło większego fontu: [assets/fonts](assets/fonts/README.md).

Scroll działa cyklicznie tylko od prawej do lewej. Polskie znaki potwierdzone przez użytkownika; wygląd czcionki zaakceptowany tymczasowo, do późniejszego dopracowania.

W ostatniej scenie diagnostycznej trzy widżety (FFT/LR/analog) pokazują jawnie syntetyczne dane TEST, przełączane automatycznie. To weryfikacja renderowania; analiza DSP i audio pozostają w dalszych etapach.
