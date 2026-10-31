# ADR0032 — lokalna konfiguracja bez logowania i HOME po połączeniu Wi-Fi

Data: 2026-10-31. Status: jawna decyzja użytkownika, implementacja gotowa do
odbioru OLED/WWW. Zastępuje auth i stały ekran konfiguracji z ADR0031.

## Potwierdzenie poprzedniego etapu

Użytkownik: „Działa”. Załączony log potwierdza STA/GOT_IP, zapisany profil,
wyłączenie AP około 5 s po IPv4, następnie AP clients=0/STA=1. Heap internal
po wyłączeniu AP stabilny 142383 B, minimum 84512 B; stos network HWM 4676 B.
Nie wywodzimy z tego testu importu/restartu katalogu ani nowej wersji OLED.
Przerwanie sesji HTTP klienta AP przy jego wyłączeniu jest spodziewane.

## Decyzja użytkownika

Usuwamy sekcję „Dostęp do konfiguracji” i dodatkowe uwierzytelnianie WWW.
Lokalny panel i odczyt/zapis konfiguracji dostępne przez AP oraz po STA
bez hasła panelu, nagłówka Authorization, Basic challenge i blokady prób.
Hasło samej sieci AP pozostaje stałe/prywatne i wyświetlane podczas AP.
To jawne zastąpienie wcześniejszego wymogu auth przed zapisem z dokumentów
10/11 i master prompt, nie brakująca zgoda wymagająca ponownego pytania.
Dostęp sieciowy do lokalnego HTTP wystarcza do zmiany konfiguracji; zapisane
hasła STA nadal nie są zwracane przez GET. Polityka przyszłej aplikacji/OTA OPEN.

Zachowujemy walidację danych, limity body, timeouty, custom header i kontrolę
Origin dla mutacji oraz brak CORS. Są niewidoczną częścią transportu formularzy,
nie logowaniem lub dowodem znajomości sekretu. Automatyczny odczyt listy sieci
oraz licznika playlisty po otwarciu strony; dostępny przycisk odświeżenia.
Usunięty callback sprawdzający hasło z network i zależność portalu od mbedtls.

## OLED i sterowanie

Ekran zależy od bieżącego stanu sieci, nie samego boot_ap_enabled:
- AP aktywny: ekran SSID/hasła/AP URL, bez gestów odtwarzacza.
- AP wyłączony i STA/IP: dotychczasowy HOME odtwarzacza, aktywne gesty enkodera.
- Brak AP i brak STA: widok ponawiania; nie pokazujemy starego IP.

Trzecia linia HOME, y24..31, pokazuje wyśrodkowane `IP: <adres IPv4>` zamiast
statusu TEST. Nazwa stacji/tytuł i dolny układ widżetów nie są przesuwane.
Adres jest kopiowany do adaptera UI, czyszczony po utracie połączenia.
Lista stacji nadal otwierana standardowym gestem; adres dotyczy widoku HOME.
Normalny widok i lokalne gesty działają bez wiązania z nieuruchomionym ownerem
audio. Nie uruchamiamy samoczynnie testowego tonu/klipu po połączeniu z Wi-Fi.
Odtwarzanie streamów i powiązanie importowanego katalogu z playerem nadal OPEN;
zmiana dotyczy ekranu i dostępu do istniejącej konfiguracji.

## Weryfikacja

222 testy hostowe PASS. Nowy test rastera: dokładne glify `IP: 10.0.0.42`
w y24..31, reszta HOME identyczna, usuwanie IP i odrzucenie za długiego/błędnego
tekstu bez zmiany poprzedniej wartości. Test HTTP: odczyt/zapis bez logowania,
nadal odrzucane obce Origin, brak znacznika żądania, błędne body i timeout.
Chromium 390px: automatyczny odczyt bez logowania, brak Authorization,
dodawanie/kolejność/zapis sieci i preview/commit playlisty PASS, dane syntetyczne.
Native ESP-IDF6.1 build PASS. Nie wykonano flash/erase/eFuse.

Odbiór nowej wersji OPEN: panel po Ctrl+F5 bez pola hasła, po STA/AP-off
HOME i IP w trzeciej linii, enkoder, ponowne otwarcie WWW pod adresem LAN.
