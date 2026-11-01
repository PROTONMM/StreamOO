# Narzędzia pomocnicze

`generate_flac_network_fixture.py` — trzy długie, oryginalne próbki FLAC
16/24bit i24bit headerless oraz lokalna playlist.csv. Hostowy FFmpeg/ffprobe,
bez zmiany firmware. Instrukcja: [FLAC_LAN_TEST](../docs/FLAC_LAN_TEST.md).
Wyniki generuj w build/, bez dodawania playlisty i mediów do repo.

`font_generator/` — generator bitmap Monaspace Krypton z TTF (Pillow), gdy pobierzesz font zgodnie z jego licencją. Można dopisać podgląd PNG i packing 4bpp.
`check_config.py` — zgrubna walidacja tekstowego pliku `sdkconfig` przed buildem; runtime walidator wymagany w etapie 02.

Nie wprowadzaj zewnętrznych plików czcionek bez ich licencji i informacji o copyright.
