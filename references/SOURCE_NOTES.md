# Materiały referencyjne i ograniczenia

- `YR.kicad_sch` — schemat użytkownika: baza domyślnych pinów. Nie zawiera pełnej odpowiedzi na pytanie o konkretny moduł OLED i jego reset.
- `myoptions.h` — konfiguracja wyjściowa yoRadio; nie odpowiada pinoutowi nowej PCB i nie jest przepisem do portowania.
- GitHub `https://github.com/PROTONMM/YR_0.3.1` — PCB 0.3.1.
- GitHub `https://github.com/VaraiTamas/yoRadio/` — dodatkowe studium rozwiązań audio; tylko inspiracja. Nie wciągać GPL bez świadomej decyzji o licencji.
- GitHub `https://github.com/githubnext/monaspace` — Monaspace Krypton, OFL-1.1; samego fontu **nie dołączono**.
- ESP-IDF docs: `https://docs.espressif.com/projects/esp-idf/` i ESP-DSP/esp_audio_codec: sprawdzić bieżącą wersję przed integracją.

Wcześniejsze dokumenty robocze z rozmowy miały propozycje niepoparte schematem. Prawidłowy pinout znajduje się w `docs/02_HARDWARE.md`. Prawdziwy format pliku stacji i dokładny plik czcionki 5×7 PL wymagają dostarczenia/wyodrębnienia fixture i weryfikacji.
