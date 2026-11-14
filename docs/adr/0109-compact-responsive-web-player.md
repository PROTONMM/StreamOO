# ADR0109 — zwarty panel WWW, motywy i format audio

Data:2026-11-14. Status:wdrożone; fizyczny Android/ESP32 OPEN.

Użytkownik wymaga minimalistycznego panelu PC/telefon, mniej przewijania,
motywów jasnego i ciemnego oraz informacji o kodeku/bitrate. Przy zmiennym
bitrate ma być tylko informacja o zmienności.

## Układ i obsługa

Usunięto powitalne „Twoje radio” i duży nagłówek. Logo, wybór motywu,
nawigacja i status połączenia zajmują zwarty pasek. Odtwarzanie, kodek,
bitrate, próbkowanie, głośność i IP są widoczne od razu. Na PC odtwarzacz
jest obok listy, do700px nad nią. Lista wszystkich stacji zachowuje
wyszukiwanie i nie ma stron. Wysokość wynika z dostępnego okna; zwykły
widok mieści się w testowanych pionowych oknach320×740 i390×844.
Długie nazwy/komunikaty, mała wysokość lub powiększenie mogą naturalnie
wydłużyć stronę, bez ukrywania kontrolek. Wąski poziomy ekran także
przewija stronę; nie blokujemy scrolla dokumentu.

Ustawienia Wi-Fi/zegara/obsługi i import są rozwijane; Wi-Fi domyślnie
otwarte w AP. Import przechodzi między konfiguracją AP a edytorem LAN.
Edycja nadal ma25 wierszy na stronę. Istniejące zapisy, potwierdzenia
usuwania, CAS i ochrona niezapisanych zmian pozostają. Suwak od razu
pokazuje wartość lokalną; status nie nadpisuje jej podczas przeciągania.
Nowy komunikat przy braku wyników wyszukiwania.

Trzy motywy: jasny, ciemny, systemowy. CSS prefers-color-scheme obsługuje
zmiany systemu w trybie auto. Wybór w localStorage `streamoo.theme`
dotyczy przeglądarki/origin, bez zapisów flash/NVS. Niedostępny storage
nie blokuje panelu. SVG logo używa kolorów motywu. Bez nowego frameworka,
fontu z Internetu, endpointów mutujących lub zmiany CSP.

## Format i bitrate

GET/api/v1/status otrzymuje dodatkowe pola audio: `codec`, `sample_rate`,
`bitrate_kbps`, `bitrate_measured`, `bitrate_variable`. Schema_version1
pozostaje, stare pola bez zmian. Codec pochodzi z obecnego snapshotu
playera (rodzina dekodera, bez rozróżnienia AAC-LC/HE; MPEG-TS pozostaje
nazwą kontenera dla sesji TS, nie zgadujemy kodeka wewnętrznego).
DTO waliduje zakończenie/znaki nazwy i limity wartości; bufor JSON768B.
Adapter kopiuje dostępny format, nie analizuje audio w request handlerze.

Bitrate deklarowany ma dotychczasowy priorytet, oszacowanie z dekodowanego
czasu PCM otrzymuje znak≈. Nie utożsamiamy oszacowania ze zmiennym bitrate.
Istniejący estymator analizuje kolejne okna>=2s źródłowego PCM. Po trzech
zmianach między oknami przekraczających jednocześnie8kb/s i5% wyższej
wartości ustawia `variable` do końca sesji/formatu. Powtarzanie statusu,
WAIT, zaokrąglenia i pojedynczy skok z powrotem nie wystarczają.
Trzy zmiany wymagają co najmniej czterech okien, około8s audio.
Flaga działa także przy nominalnej deklaracji icy/HLS. STOP, utrata sesji
lub zmiana formatu zerują stan. Obliczenia nadal poza ścieżką I2S.

WWW pokazuje wtedy wyłącznie „Zmienny”, bez liczby. Brak formatu/STOP/
offline daje kreski. To wykrycie obserwowanej zmienności z estymatora,
nie gwarantowana klasyfikacja VBR z nagłówków ramek: małe zmiany mogą
pozostać niewykryte, a narzut kontenera może wpłynąć na pomiar. Nie opisujemy
niewykrytej zmienności jako potwierdzonego CBR. Parametry wymagają odsłuchu
stacji rzeczywistych; nie zgadujemy VBR na podstawie rozszerzenia URL.

## Weryfikacja

- 499 testów hostowych PASS, bez skipów. Nowe przypadki obejmują
  stabilny pomiar, padding, WAIT, pojedynczy skok, wykrycie i reset,
  przekazanie flagi przez player także z deklaracją serwera, format JSON,
  błędne nazwy/limity, pojemność768B oraz widok Zmienny/≈/brak danych.
- `python3 tools/check_portal_browser.py`: Chromium390px, AP/LAN,
  wybór stacji, volume/STOP/PLAY, add/edit/delete/export/import/clear/CAS,
  Wi-Fi/zegar/obsługa i motywy PASS. Dane wyłącznie syntetyczne.
- `python3 tools/check_portal_layout.py`: Chromium1280×900 jasny,
  390×844 ciemny,320×740 jasny,844×390 ciemny,390×844 systemowy z wymuszonym
  ciemnym systemem PASS. 2048 pozycji, filtr ostatniej stacji, zachowanie
  scrolla i węzłów DOM przy pollingu, brak poziomego overflow także po
  rozwinięciu ustawień, przyciski44px, zapis/odczyt motywu i storage denied.
  PNG obejrzane; narzędzie zapisuje je w/tmp/streamoo-*.png.
- Native ESP-IDF6.1/esp32s3 build PASS:0x175f60B,90% app wolne.
  Node --check, GPIO/platform i git diff --check PASS.

Przeglądarka hostowa nie zastępuje testu dotyku na fizycznym Androidzie
ani odczytu danych z grającego ESP32. Bez flash agenta. Do odbioru:
PC i telefon, oba motywy/auto, duża lista, keyboard/zoom, AP→LAN,
MP3/AAC/FLAC i zmiany bitrate, STOP/PLAY/zmiana stacji bez starego formatu.
