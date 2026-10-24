# Punkt wznowienia StreamOO

Zapis na koniec pracy: 2026-10-24. Użytkownik zakończył pracę na dziś.

## Gdzie jesteśmy

Etap **06a — podstawowe wyjście I2S/PCM5102A** zaimplementowany i odebrany
przez użytkownika („działa”). Kod: `edb7c63`, odbiór: `80e146c`.
Gałąź: `feature/streamoo-implementation`.

Działające i odebrane na płytce:

- SSD1322: HOME, lista stacji, polskie znaki, scroll wyłącznie w lewo,
  trzy widżety z danymi syntetycznymi. Wygląd czcionki do dopracowania później.
- Enkoder półkrokowy: **A=5, B=6, BTN=7**, REVERSED=y — prawo zwiększa.
  Schemat referencyjny ma B16, ale rzeczywisty sprzęt użytkownika ma B6.
- Gesty trybu A i interaktywny test UI: volume, lista/confirm/cancel,
  PLAY/STOP, pressed-rotate zmienia widżet bez zmiany volume.
- NVS: volume i widżet zapisują się po 5s bez zmian i wracają po restarcie.
- I2S: DOUT48/BCLK47/WS21, 48kHz stereo32, Philips, bez MCLK.
  Jednorazowa sekwencja około10s: cisza/L1k/R2k/oba1k/cisza, peak ~-30dBFS,
  rampy50ms. Wydawanie próbek przez callback DMA w IRAM, stan/tablica w DRAM.

**Obecny TEST VOL/PLAY nie steruje jeszcze rzeczywistym dźwiękiem.**
Ton jest niezależną skończoną sekwencją startową. UI nadal używa testowego
katalogu i syntetycznych poziomów. Nie ma sieci, strumieni, dekoderów,
produkcyjnego playera, analizy DSP ani WWW.

## Od czego zacząć jutro

Kolejny mały zakres: **06b — rzeczywisty gain z rampą i PLAY/STOP**.

1. Przeczytać AGENTS.md, CODEX_MASTER_PROMPT.md, indeks, STATUS oraz
   docs/06_AUDIO.md i ADR0018. Nie powtarzać odebranego bring-upu od zera.
2. Zaprojektować oddzielnego właściciela stanu audio/komend; UI wysyła
   nieblokujące kopie poleceń. Dotychczasowy encoder_demo to tylko test.
3. Dodać i przetestować gain 0..100 (0 mute,100 unity), zmianę bez skoków
   przez rampę w próbkach oraz płynne PLAY/STOP na źródle testowym.
   Krzywa -60..0dB i czas rampy30–100ms są propozycjami do strojenia.
4. Połączyć sterowanie enkodera i przywrócony volume z rzeczywistym testowym
   audio; nadal nie twierdzić, że jest streaming lub gotowy pełny player.
5. Build/testy, STATUS/ADR, osobny commit; potem odbiór na płytce.

## Zweryfikowany stan i polecenia

Ostatnia weryfikacja firmware: **107 testów PASS**, ASan/UBSan generatora PASS,
ESP-IDF6.1 reconfigure/build PASS. LeakSanitizer wyłączony pod ptrace;
nie deklarować testu wycieków. Ostatni odbiór audio jest potwierdzeniem
użytkownika, bez końcowego logu/pomiarów max_fill, jitter i underrun.

```sh
python3 -m unittest discover -s tests -q
source /home/mariuszk/.espressif/tools/activate_idf_v6.1.sh
export PATH=/usr/bin:$PATH
idf.py build
```

Domyślnie włączone STREAMOO_ENCODER_UI_TEST oraz STREAMOO_I2S_SMOKE_TEST.
Wyłączenie pierwszego przywraca skończony pokaz OLED; drugiego — usuwa
ton przy boot. NVS nie kasować przy wgrywaniu. Agent nie flashuje płytki.

Pozostałe otwarte kwestie: finalna czcionka/licencja zmian PL, wrap listy,
format czasu/strefa, liczba kroków na ząbek i graniczna szybkość wejść,
kontrolowany shutdown/flush NVS, testy zaniku podczas zapisu, dokładny
model/zwory DAC i pomiary audio. Nie traktować ich jako zakończonych.

Nie dodawać trailerów AI-Model/AI-Tokens do commitów ani wymyślonych metryk.
Nie wykonywać alokacji, flash, FFT, logowania ani blokującego I/O w ścieżce
wydawania próbek. Bez Arduino i kopiowania API yoRadio.
