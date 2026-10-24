# Font generator

To adaptowany skrypt roboczy konwertujący TTF na dane używane z SSD1322 (Pillow). Monaspace Krypton należy uzyskać z oficjalnego repozytorium zgodnie z OFL-1.1; gotowego fontu nie dołączono. **Przed wdrożeniem** zweryfikuj format wygenerowanych danych, obsługę metryk, 4bpp i własny renderer, ponieważ narzędzie jest punktem wyjścia, a nie zweryfikowanym pipeline produkcyjnym.

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install Pillow
python tools/font_generator/ttf_to_ssd1322.py --help
```
