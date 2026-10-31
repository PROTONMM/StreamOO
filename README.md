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

Testy hostowe: `python3 -m unittest discover -s tests -v`. Build: `idf.py reconfigure build`. Dla modułu msalamon R5/R8 dodano test OLED (około 46 s, potem panel OFF). Odbiór resetu i obrazu: [protokół sprzętowy](docs/HARDWARE_BRINGUP.md).

Podstawowe działanie OLED potwierdzone przez użytkownika; nowa scena renderera i szczegółowe pomiary wymagają odbioru sprzętowego. Dołączono mały font PL i jego test; większy font na bazie Krypton i marquee są w teście diagnostycznym; pełny UI nadal w implementacji. Stan licencji małego fontu: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

Model i tokeny nowych commitów: [procedura i dostępność danych](docs/COMMIT_METRICS.md). Źródło większego fontu: [assets/fonts](assets/fonts/README.md).

Scroll działa cyklicznie tylko od prawej do lewej. Polskie znaki potwierdzone przez użytkownika; wygląd czcionki zaakceptowany tymczasowo, do późniejszego dopracowania.

W ostatniej scenie diagnostycznej trzy widżety (FFT/LR/analog) pokazują jawnie syntetyczne dane TEST, przełączane automatycznie. To weryfikacja renderowania; analiza DSP i audio pozostają w dalszych etapach.

Na końcu diagnostyki jest automatyczny pokaz animowanej listy TEST. Właściciel UI oddziela przeglądanie od polecenia wyboru; rzeczywista lista/player oraz obsługa enkodera pozostają w dalszych etapach.

HOME ma pełny układ stacji/metadanych i dolnych paneli. Diagnostyka pokazuje syntetyczne warianty TEST: komplet danych, odrzucenie starych metadanych, nowe dane i offline; nie jest to jeszcze połączenie z siecią ani playerem.

Etap 05c domyślnie uruchamia interaktywny test enkodera/UI od razu zamiast
skończonego pokazu OLED. TEST VOL/PLAY/W pokazuje skutki gestów, nadal bez audio.
Od etapu 05d volume/widżet są zapisywane w NVS po 5s bez zmian.
Wyłącz STREAMOO_ENCODER_UI_TEST, aby wrócić do pokazu 46s.
Instrukcja odbioru: [HARDWARE_BRINGUP](docs/HARDWARE_BRINGUP.md).

Etap06a domyślnie dodaje jednorazowy test dźwięku PCM5102A przy boot:
około10s, lewy1kHz, prawy2kHz i oba kanały, z przerwami i rampami.
Stały niski poziom, niezależny od TEST VOL/PLAY. Wyłącz
STREAMOO_I2S_SMOKE_TEST, aby nie uruchamiać tonu. To nie jest streaming radia.

Etap06b domyślnie zastępuje skończony ton testem sterowanym enkoderem:
start w STOP, rzeczywiste VOL i PLAY/STOP z rampą50ms. Źródło to ciągły
cichy ton1kHz stereo; katalog i widżety nadal TEST. Wyłącz
STREAMOO_AUDIO_CONTROL_TEST, aby przywrócić test06a.

Etap06c dodaje FIFO PCM160ms i niezależne odgałęzienie przed gain.
Brak zapasu uruchamia wyciszenie/rebuffer; przeciążony tap odrzuca bloki
bez zatrzymywania audio. Konsola podaje liczniki i debug peak przed volume.
Opcje testowe stall/tap-delay są domyślnie wyłączone. Widżety nadal TEST.

Etap06d domyślnie wybiera własny plik MP3 zamiast sinusa. Na starcie
ESP32-S3 dekoduje klip do PSRAM; PLAY odtwarza zapętlone sygnały kanałów
z przerwami. Log MP3 PoC i odsłuch są nowym testem sprzętowym. Brak sieci
i resamplingu; widżety TEST. Licencje komponentu w THIRD_PARTY_NOTICES.

Bieżący06g: AAC-LC dekodowane podczas odtwarzania przez oddzielny interfejs
źródła i testowy pierścień skompresowanych danych4KiB w PSRAM. Pełny bufor
wstrzymuje zapis, chwilowy brak danych zachowuje częściowy blok PCM.
Nadal własny klip testowy, bez sieci; start STOP, VOL/PLAY rzeczywiste.
Bieżący test i punkt wznowienia: [RESUME](docs/RESUME.md).

06h przenosi testowy czytnik pliku do osobnego zadania. Mutex i token generacji
chronią pierścień przed współbieżnym dostępem i spóźnionymi danymi po rewind.
Dekoder nadal ma jednego właściciela; sieć nie jest jeszcze zaimplementowana.
