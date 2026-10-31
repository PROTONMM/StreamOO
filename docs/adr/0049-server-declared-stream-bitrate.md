# ADR0049 — bitrate deklarowany przez serwer

Data:2026-10-31. Użytkownik kontynuuje, fizyczny test trwałości strefy
wykona w międzyczasie. Nie blokujemy kolejnych kroków na tym odbiorze.

Reader HTTP przechwytuje opcjonalne icy-br, integer1..4096kb/s. Błędne,
zerowe lub przekraczające limit wartości oznaczają nieznane, bez wpływu
na odtwarzanie. Reset przed każdym redirect i nową sesją. To nominalna
deklaracja serwera, nie pomiar VBR/przepustowości i nie dowód poprawności
konfiguracji nadawcy. Jednostkę sprawdzono w pierwotnym API OpusServerInfo:
https://www.opus-codec.org/docs/opusfile_api-0.5/structOpusServerInfo.html .
Nie importowano kodu Opus/Icecast ani nowych zależności.

Player przekazuje bitrate do UI dopiero przy znanym formacie PCM/kodeku,
STOP/retry/offline/retiring czyści pole. Dolny lewy panel pokazuje np.192k
pod MP3/AAC; brak wiarygodnego nagłówka nadal --. Nie zastępujemy bitrate
sample rate i nie liczymy go z napełnienia bufora. Bez zmian w ISR/audio/DSP.

332 testy hostowe PASS. Trzy nowe scenariusze rzeczywistego readera z
atrapą transportu: poprawne icy-br, błędne opcjonalne wartości nie blokują
audio, nagłówek odpowiedzi302 nie wycieka do końcowego strumienia.
Actor i UI sprawdzają publikację/czyszczenie. HTTP/actor ASan/UBSan PASS
(LSan off). Sprzętowy bitrate i NVS power-cycle nadal OPEN.

Native ESP-IDF6.1 build PASS,0x161080 (6% wolne), GPIO validator
i diff --check PASS. Bez nowych zależności, flash/erase/eFuse i zmian partycji.
