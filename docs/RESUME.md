# Punkt wznowienia StreamOO

Aktualizacja: **2026-10-31, 08a: katalog stacji na OLED gotowy do odbioru**.
Gałąź feature/streamoo-implementation. Szczegóły STATUS i ADR0033.
Użytkownik potwierdził nowy HOME/IP z 09d.

## Potwierdzony stan

Użytkownik potwierdził działanie 09c. Log: STA/GOT_IP, zapisany profil, AP
wyłączony po około 5 s; AP clients=0/STA=1. Heap internal 142383 B stabilny,
minimum 84512 B; network stack HWM 4676 B. Nie ponawiać diagnostyki UART:
wcześniejsze nieczytelne znaki były potwierdzonym efektem poruszania przewodów.

Gotowe: do 8 uporządkowanych sieci, NVS, STA/fallback/rollback, AP-off,
import CSV preview/zatwierdzenie/atomowa podmiana LittleFS. Prywatny root
/playlist.csv ignorowany/nieśledzony, nie kopiować do repo/build/testów.
Parser na RAM-kopii: 267 rekordów/11 normalizacji URL/3 zachowane duplikaty.
Nie zmieniać oryginału. Odbiór reboot/importu na PCB jeszcze bez osobnego dowodu.

## Nowa decyzja i kod 09d

Użytkownik usunął wymóg dodatkowego logowania WWW: lokalny panel AP/LAN
dostępny bez hasła panelu, Basic/Authorization i sekcji dostępu. To jawnie
zastępuje stare wymagania auth D18/D29/ADR0031; nie pytać ponownie o zgodę.
Hasło samej sieci AP nadal stałe/prywatny build i OLED podczas AP.
Zachowane Origin/custom header, limity, walidacja i brak zwracania haseł STA.
WWW samo odczytuje zapisane ustawienia.

Po STA i AP-off OLED wraca do HOME odtwarzacza, IP w trzeciej linii y24..31,
gesty aktywne. Nie wiązać UI z nieuruchomionym ownerem audio. Boot konfiguracji
nie startuje samoczynnie testowego tonu/klipu. W profilu sieciowym lista pochodzi już z playlisty w LittleFS (08a). Wybór
pokazuje nazwę na HOME, bez uruchomienia streamu i bez fikcyjnych pomiarów.
Katalog odświeża się do 1 s po commit; refcount snapshotów chroni stare nazwy.
ID name/URL/wystąpienie stabilne po reorder/reboot, granice edycji w ADR0033.

226 testów hostowych PASS; store/controls również ASan/UBSan PASS, bez TSan.
Native build PASS. Poprzedni Chromium formularzy bez logowania zaliczony. Flash/erase/eFuse nie wykonano.
Odbiór: zwykły idf.py -p PORT flash; po STA sprawdź listę zapisanej playlisty,
wybór i polskie nazwy/IP. Import podczas otwartej listy ma zamknąć ją/odświeżyć
katalog bez restartu. Reboot zachowuje katalog, nie zapamiętuje jeszcze wyboru.
Sprawdź heap i stosy. NETWORK_BRINGUP, ADR0033.
Partycje z 09c bez dalszej zmiany: zachowane NVS/PHY/app i storage 1 MiB.
Nie kasować flash, nie wgrywać obrazu FS. Użyć istniejącego prywatnego klucza,
nie pytać ponownie i nie wypisywać go.

## Dalszy zakres

Player HTTP(S)/ICY, spięcie wyboru z playerem i zapis ostatniego ID NVS,
DSP, mDNS, OTA nadal przyszłe; nie twierdzić, że konfiguracja już odtwarza radio.
Pilot RC5 i aplikacja telefonu przez API ustalone jako przyszłe (ADR0029),
brak modelu/GPIO/mapy klawiszy i kontraktu mobilnego. MicroSD przyszłe, bez
wymyślania pinów. AP bez odtwarzania pozostaje wymaganiem.

Odebrane: PL font (estetyka i licencja modyfikacji OPEN, użytkownik zgodził
się kopiować tymczasowo), scroll tylko w lewo, encoder półkrok A5/B6/BTN7,
REVERSED=y, preferencje NVS, PCM/I2S/stress oraz lokalny MP3/AAC odsłuchem.
Dokładna historia w STATUS/ADR0012–0032. Nie dodawać AI-Model/AI-Tokens
do commitów; metryki tylko ze zweryfikowanego źródła, bez zgadywania.
