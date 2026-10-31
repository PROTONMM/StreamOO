# StreamOO — propozycja logo OLED

Oryginalny znak: dwa przenikające się O i linia sygnału; napis StreamOO,
podpis INTERNET RADIO. Native SSD1322:256×64,16 poziomów szarości, czarne tło.
Nie jest obrazem generowanym AI ani bitmapą pobraną z zewnątrz. Renderer
`components/ui/streamoo_ui_boot.c` używa istniejącego fontu Monaspace Krypton
(OFL1.1, zob. assets/fonts/README.md i THIRD_PARTY_NOTICES.md).

`startup-256x64.png` to dokładne wyjście renderera, nie dodatkowy asset
ładowany przez firmware. Odtwarzanie podglądu:

```
python3 tools/preview_boot_logo.py assets/branding/startup-256x64.png
python3 tools/preview_boot_logo.py /tmp/streamoo-startup-preview.png --scale 4
```

Logo jest propozycją do oceny na OLED; wygląd/rozmieszczenie można zmienić
niezależnie od odtwarzacza. Wyświetlane1,5s po zimnym starcie/resecie,
bez powtórzenia przy PLAY, reconnect Wi-Fi, AP lub zmianie stacji.
