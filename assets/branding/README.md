# StreamOO — logo

Jeden znak: autorski geometryczny napis „Stream” i dwie połączone pętle „OO”,
przypominające nieskończony strumień. Zaokrąglone kreski i wygładzone krawędzie.
Bez importowanych ikon, obrysów fontów ani obrazów zewnętrznych.

- `streamoo.svg`: skalowalny znak, grafitowy napis i zielone OO dla WWW.
- `startup-256x64.png`: dokładne wyjście renderera OLED SSD1322, 16 szarości.
- `startup-preview-4x.png`: powiększenie tego samego framebuffer bez interpolacji.

Wspólne ścieżki definiuje `tools/generate_brand_mark.py`. Generator zapisuje SVG,
wstawia ten sam SVG do nagłówka portalu i generuje maskę4bpp224×40 w
`components/ui/streamoo_boot_wordmark.inc` (4480B const w flash).
Wymaga wyłącznie na hoście pycairo (sprawdzono1.25.1); firmware i WWW
nie potrzebują Cairo ani pobierania zasobów z Internetu. Normalny build
korzysta z już wygenerowanej maski. Rasteryzacja4× odbywa się offline.

```sh
python3 tools/generate_brand_mark.py
python3 tools/preview_boot_logo.py assets/branding/startup-256x64.png
python3 tools/preview_boot_logo.py assets/branding/startup-preview-4x.png --scale 4
```

Renderer nakłada maskę na istniejący framebuffer, bez heap i dodatkowego
framebuffer. Podpis INTERNET RADIO używa obecnego Misc Fixed5×8/public domain.
Logo pojawia się minimum1,5s po zasileniu/resecie i pozostaje podczas startu
Wi-Fi/autoplay według ADR0068. Nie powraca przy PLAY ani reconnect.
Nowy wygląd wymaga oceny na fizycznym OLED; ADR0108 zastępuje grafikę ADR0063.
