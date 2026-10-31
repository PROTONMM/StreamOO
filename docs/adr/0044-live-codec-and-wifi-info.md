# ADR0044 — rzeczywisty kodek i RSSI w HOME

Data: 2026-10-31. Status: wdrożone, odbiór OLED OPEN.

Użytkownik po9cd7306: dźwięk OK, słupki lepsze; pozostawić do dopracowania
i kontynuować. Odsłuch i poprawa wyglądu potwierdzone, bez nowych logów
czasu FFT/CPU. Nie zmieniamy teraz parametrów FFT/RMS ani układu.

## Zmiana

Dolny lewy panel HOME otrzymuje dane od jawnych ownerów. Player publikuje
MP3/AAC po otwarciu dekodera i poznaniu zaakceptowanego formatu PCM,
nie po samym nagłówku HTTP. Przy STOP/retiring/braku sieci codec pusty.
AAC to nazwa rodziny, nie potwierdzenie HE-AAC/AAC+ ani profilu streamu.
Bitrate pozostaje nieznany (--); nie zastępujemy go sample rate ani
estymacją z rozmiaru bufora. Rate dekodera pozostaje w diagnostyce.

Worker sieci przy udanym połączeniu odczytuje RSSI z wifi_ap_record_t,
następnie odświeża przez esp_wifi_sta_get_ap_info najwyżej raz na sekundę.
UI nigdy nie wywołuje SDK Wi-Fi. Błąd odczytu unieważnia RSSI, bez
wymuszania rozłączenia; brak STA natychmiast maskuje ważność w getterze.
SDK dopuszcza rzadkie dodatnie RSSI przy mocnym sygnale: clamp do0dBm,
aby zachować kontrakt renderera [-127,0]. -128 traktowane jako nieznane.
Ikona słupkowa i liczba dBm korzystają z istniejącego renderera.

Adapter UI kopiuje krótką nazwę kodeka, waliduje RSSI/tekst przed zmianą
stanu, bez alokacji i I/O. Zmiana RSSI nie resetuje marquee. Geometria,
wiersz IP/VOL, preferencje i audio/DSP bez zmian. Nie dodajemy zależności.

## API i testy

Sprawdzone lokalne API przypiętego ESP-IDF6.1: esp_wifi_sta_get_ap_info,
wifi_ap_record_t.rssi i uwaga o dodatnich wartościach w
components/esp_wifi/include/esp_wifi_types_generic.h. To już używany
komponent SDK, bez zmiany wersji/licencji/importu kodu.

307 testów hostowych PASS. Owner sieci:13 scenariuszy, nowy sprawdza
pierwszy RSSI, okresowe zmiany, dodatni clamp i błąd SDK; stare scenariusze
sprawdzają nieważność RSSI po rozłączeniu. Actor: kodek MP3 po PCM i
czyszczenie przy STOP/retry/braku sieci. UI: kopiowanie MP3/AAC, poprawność
snapshotu, nieznany bitrate, zmiana RSSI, walidacja atomowa i czyszczenie.
Network/actor/controls ASan/UBSan PASS (LSan off, bez TSan).
Native ESP-IDF6.1 build PASS,0x15f690 (6% wolne), check_config i diff PASS.
Bez flash/erase/eFuse/zmian partycji. Sprzętowe nowe pole RSSI/kodek OPEN.
