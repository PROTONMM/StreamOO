# 09 — Wi-Fi provisioning i mDNS

**USTALONE:** przy pierwszym uruchomieniu bez danych STA urządzenie tworzy SoftAP z unikatową nazwą `StreamOO-XXXXXX` opartą na MAC. **Hasło AP jest stałe, wspólne dla wszystkich urządzeń** (przekazywane poza repo); WPA2-Personal. Portal WWW pod `http://192.168.4.1`; po pomyślnym połączeniu z Wi-Fi STA wyłącz SoftAP. Użytkownik może później zmienić Wi-Fi przez stronę WWW.

## Przepływ
- Brak credentiali → SoftAP → skan SSID, wybór, hasło → tymczasowy STA test → dopiero po powodzeniu zapis do NVS → `WIFI_MODE_STA`, AP wyłączony.
- Zła konfiguracja → portal AP pozostaje aktywny, stary profil pozostaje dostępny; strona komunikuje błąd.
- Przy zmianie Wi-Fi przez WWW: przejściowe rozłączenie przeglądarki jest spodziewane; procedura rollback do ostatnich działających danych po nieudanej zmianie.
- Krótki zanik STA nie uruchamia AP automatycznie; retry STA i UI offline. Awaryjne uruchomienie AP w menu jako przyszły mechanizm (gest/menu do implementacji).

## mDNS
- Host domyślny `streamoo-xxxxxx.local` na bazie unikalnego sufiksu MAC, **nie** wspólny `streamoo.local`.
- Użytkownik może zmienić nazwę przez WWW; zapis w NVS, walidacja DNS hostname i obsługa konfliktów.
- Reklamuj HTTP przez DNS-SD `_http._tcp`; po zmianie Wi-Fi odśwież usługę.

## Bezpieczeństwo
Wspólne hasło AP zwiększa ryzyko nieautoryzowanego dostępu; ogranicz czas/warunki aktywacji AP, nie ujawniaj hasła w logach, zabezpiecz panel administracyjny. Preferuj proces build/secrets dla wartości wspólnej, bez umieszczania jej w Git.

## Pierwszy zakres09a — test SoftAP

Natywny AP WPA2 i DHCP w osobnym zadaniu; hasło podawane lokalnie przez
`tools/configure_network.py`, plik poza repo. Bez klucza Wi-Fi pozostaje
wyłączone. Na tym etapie brak portalu/STA/mDNS i zapisu credentials do NVS;
pełny przepływ powyżej pozostaje wymaganiem. Instrukcja:
[NETWORK_BRINGUP](NETWORK_BRINGUP.md), [ADR0026](adr/0026-secured-softap-bringup.md).
Nowy profil partycji SDK1500KiB zachowuje położenie NVS/PHY/factory.
Odbiór RF/DHCP i współpracy z audio OPEN.

## Zakres09b — portal odczytu i ekran AP

Statyczny portal HTTP192.168.4.1 i GET/api/v1/status, bez zapisów ustawień.
OLED podczas AP wyświetla SSID i pełne hasło, zgodnie z poleceniem użytkownika.
Stałe hasło nadal wbudowane z prywatnego buildu, niewysyłane przez HTTP/logi.
Użytkownik potwierdził AP off po udanym STA z IP; przejście to będzie wdrożone
wraz z zapisem/testem STA, nie po samym dołączeniu klienta AP.
[ADR0027](adr/0027-readonly-portal-and-ap-oled.md). Odbiór09b OPEN.

Nowsza decyzja: konfiguracja AP bez odtwarzania. Boot pomija cały testowy
pipeline audio, enkoder w AP nie steruje playerem, portal pokazuje audio off.
To zastępuje eksperymentalną współpracę lokalnego AAC z AP. MicroSD później,
niezaimplementowane; przyszłe dynamiczne STA→AP wymaga kontrolowanego STOP.
[ADR0028](adr/0028-ap-setup-without-audio.md).
