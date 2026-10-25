# Oryginalne pliki testowe MP3 i AAC-LC

channels_48k_stereo.mp3 zawiera wyłącznie matematycznie wygenerowane sygnały,
bez cudzych nagrań, muzyki ani bitmap/kodu yoRadio. Generator:
`python3 tools/generate_mp3_fixture.py` (wymaga FFmpeg z libmp3lame na hoście).
FFmpeg/libmp3lame służą tylko do tworzenia danych testowych; ich kod i biblioteki
nie są kopiowane ani linkowane do firmware.

4s źródła stereo signed16/48kHz, peak1036 (~-30dBFS), rampy50ms:
0–0.5s cisza;0.5–1.5s lewy1kHz;1.5–2s cisza;2–3s prawy2kHz;
3–3.5s oba1kHz;3.5–4s cisza. Kompresja MP3 CBR128kbps, bez ID3/Xing.
Plik ma64512B/168 ramek MPEG1 LayerIII po1152 próbki.

JSON zapisuje wersję narzędzia, SHA pliku i referencyjnego PCM z FFmpeg.
Referencja ma193536 ramek zamiast192000 źródłowych (padding/priming kodeka).
Nie wymagamy bitowej identyczności różnych implementacji dekodera.
Firmware kontroluje metadane, progres, zakres liczby ramek oraz peak,
a właściwy dekoder Espressif wymaga uruchomienia na ESP32-S3.

Klip w firmware jest zapętlany z PCM przygotowanego w PSRAM; START/STOP to
sterowanie gainem źródła testowego, nie pełny player plików/radia. PLAY
może zacząć w dowolnym miejscu cyklu, bo źródło biegnie także w STOP.

## AAC-LC w ADTS

`python3 tools/generate_aac_fixture.py` używa wspólnego oryginalnego źródła
`synthetic_pcm()` i natywnego enkodera AAC FFmpeg, profile aac_low, cel128kbps
(nie gwarancja CBR). Plik channels_48k_stereo.aac ma37039B,189 ramek ADTS
po1024 próbki; MPEG4 AAC-LC,48kHz, stereo. Referencja193536 ramek PCM;
SHA i wersja narzędzia w channels_48k_stereo_aac.json. Test hostowy sprawdza
SHA, długość i nagłówki każdej ramki. Runtime dekodera wymaga ESP32-S3.
