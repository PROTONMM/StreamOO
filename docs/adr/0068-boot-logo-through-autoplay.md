# ADR0068 — logo do uruchomienia odtwarzania

Data:2026-10-31. Status: wdrożone; odbiór OLED/audio OPEN.

Użytkownik odrzuca ekran łączenia Wi-Fi i późniejsze wygaszenie po logo.
Wymaga dłuższego logo, a następnie automatycznego odtwarzania.

Logo zachowuje minimum1500ms z ADR0063 i pozostaje jako już wysłana klatka
podczas pierwszego STA. Owner UI nadal obsługuje snapshoty, katalog i gesty;
nie wysyła ponownie logo co40ms i nie blokuje sieci/playera.
Po IPv4 czeka minimum500ms na synchronizację. PLAYING odsłania HOME;
RETRY lub limit30s od pierwszego IPv4 odsłania normalne informacje o błędzie.
AP ma pierwszeństwo nad oczekiwaniem. Po zakończeniu startowej sekwencji
logo nie wraca na reconnect/zmianę stacji/PLAY. Gest użytkownika może je
zakończyć, więc długi STOP zachowuje wygaszanie OLED.

Domyślny autoplay=true. Settings blobv2 ma identyczną długość68B i pola;
odczyt legacyv1 wymusza autoplay=true, zachowując hostname/listę/ID.
Nowy zapis używav2, więc późniejszy świadomy wybór STOP przez WWW działa.
Odczyt migracyjny nie zapisuje flash; normalny zapis ustawień utrwala nową
wersję. Nie kasujemy NVS ani nie zmieniamy partycji. Odtwarzana jest ostatnia
istniejąca stacja; brak ID/pusty katalog wymagają wyboru, bez zastępczej stacji.

381 testów hostowych PASS. Test granic AP/offline/500ms/30s używa rzeczywistej
funkcji polityki UI; settings sprawdza default, legacyfalse→true, v2false,
zapisv2 i dotychczasową walidację/błędy/debounce. Native ESP-IDF6.1 build
PASS0x17bc90 (90% app wolne), GPIO/diff PASS. Bez nowych zależności i zmian
ścieżki I2S. Agent nie flashował. Odbiór S3: reset→logo→audio/HOME, offline→AP,
STOP/PLAY i reconnect bez powrotu logo. Hardware pozostaje OPEN.
