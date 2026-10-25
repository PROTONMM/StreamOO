# Punkt wznowienia StreamOO

Aktualizacja: **2026-10-25, etap 06b odebrany funkcjonalnie przez użytkownika**.
Kod: `a62eeb6`; potwierdzenie: „Działa, ładnie”.
Gałąź: feature/streamoo-implementation. Szczegóły i wyniki w STATUS.md.

## Odebrane na płytce

- OLED: HOME/lista, polskie znaki, scroll wyłącznie w lewo, trzy widżety.
  Wygląd czcionki do dopracowania później.
- Enkoder półkrokowy **A5/B6/BTN7**, REVERSED=y: prawo zwiększa. B6 to
  potwierdzony sprzęt użytkownika, wyjątek od B16 w schemacie referencyjnym.
- Gesty trybu A i interaktywny UI. NVS volume/widżetu po5s i restart.
- Audio06a: skończony test L1k/R2k/oba1k na I2S DOUT48/BCLK47/WS21,
  Philips stereo32/48kHz, bez MCLK. Kod edb7c63, odbiór80e146c.

## Nowe w06b — odebrane funkcjonalnie

Nowy domyślny STREAMOO_AUDIO_CONTROL_TEST zastępuje skończony test
ciągłym tonem1kHz stereo. Start w **STOP**, volume wraca z NVS.
Długi klik (tryb A) przełącza rzeczywiste PLAY/STOP, obrót zmienia gain,
VOL0 to cisza. Rampa50ms, gain100 unity. Źródło ma peak~-30dBFS przed
gain; przy VOL50 dochodzi około30dB tłumienia — do odsłuchu ustaw80–100.
Wyłącz AUDIO_CONTROL_TEST, aby wrócić do starego testu06a.

Osobny właściciel przyjmuje VolumeDelta/Toggle przez kolejkę16, UI czyta
snapshot. Czysty audio_gain używa Q16 i rampy2400 ramek bez float. Callbacki
i gain są w IRAM, stan/tablice w DRAM; brak helperów flash w obliczeniach.
UI jest jedynym producentem preferencji; revision zachowuje reset debounce
przy sklejonych zmianach. ADR0019 opisuje architekturę i granice.

**Katalog i widżety nadal są syntetyczne. Nie ma sieci, strumieni,
dekoderów, PCM FIFO, produkcyjnego playera, analizy DSP ani WWW.**
Potwierdzenie testowej stacji zmienia nazwę, ton pozostaje1kHz.
STOP pozostawia I2S z zerami, nie jest pauzą strumienia. D12 autoplay
radia nie rozstrzygnięte; kontrolowany shutdown/flush NVS nadal OPEN.

## Następny krok

1. Kolejny mały zakres: PCM FIFO i tap przed gain, przygotowanie źródła
   PCM/dekoderów, bez deklarowania gotowego streamingu.
2. Nie powtarzać odbioru funkcjonalnego06b od zera. Pomiary max_fill,
   jitter/latencji/underrun i próby obciążeniowe nadal pozostają OPEN.

Ostatnia weryfikacja: **121 testów PASS**, ASan/UBSan gain/model/source
i UI/control PASS, ESP-IDF6.1 build PASS. LeakSanitizer pod ptrace wyłączony;
bez testu wycieków. Brak pomiarów hardware jitter/underrun/latencji.

```sh
python3 -m unittest discover -s tests -q
source /home/mariuszk/.espressif/tools/activate_idf_v6.1.sh
export PATH=/usr/bin:$PATH
idf.py build
```

Przed zmianami czytaj AGENTS, CODEX_MASTER_PROMPT, indeks, właściwy etap
i STATUS. Nie powtarzaj odebranych kroków. Małe commity + test/build/ADR.
Bez Arduino i kopiowania API yoRadio. Nie kasuj NVS ani nie flashuj płytki
za użytkownika. Bez AI-Model/AI-Tokens i wymyślonych metryk w commitach.
Nie wykonuj alokacji, flash, FFT, logowania ani blokującego I/O w próbkach.

Pozostają otwarte: finalna czcionka/licencja zmian PL, wrap listy, format
czasu/strefa, kroki/ząbek i graniczna szybkość wejść, shutdown/flush NVS,
zanik podczas zapisu, dokładny model/zwory DAC i pełne pomiary audio.
