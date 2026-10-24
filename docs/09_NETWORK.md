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
