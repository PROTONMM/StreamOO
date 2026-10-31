# 11 — Przechowywanie, OTA, bezpieczeństwo

Stan implementacji 2026-10-31: Implementacja 09c: wersjonowany blob profili NVS i atomowa podmiana playlisty LittleFS, nowa storage 1 MiB. Lokalna konfiguracja WWW bez dodatkowego logowania na polecenie użytkownika (ADR0032), zapisane hasła nie są zwracane. Polityka przyszłej aplikacji OPEN. OTA wyłączone decyzją użytkownika (ADR0055). Trwałość i ograniczenia: ADR0031.

## NVS (USTALONE)
- Wi-Fi credentials, wybrany wariant runtime jeśli dopuszczony, aktualna głośność, ostatnia stacja stable_id, wizualizacja, własna nazwa mDNS, stan startowy.
- Każda zmiana głośności lub wizualizacji zeruje timer; **zapis po 5 sekundach bez nowych zmian**, gdy różni się od ostatniego trwałego stanu. Flush podczas kontrolowanego restartu; przy nagłym zaniku zasilania można utracić ostatnie 5 s zmian.
- Wybór trybu enkodera A/B jest domyślnie build-time `menuconfig`, nie opcją dynamiczną.

## LittleFS
Lista stacji, ewentualnie WWW assets, tymczasowy staging importu. 16MiB flash: jedna duża factory (offset0x10000, size0xEF0000) i LittleFS1MiB na końcu (offset0xF00000). OTA nie jest planowane, zgodnie z ADR0055. Zastosuj wersjonowanie formatu stacji i kopię/rollback.

## Sekrety i sieć
- Hasło AP stałe w produktach, ale NIE commitowane do `sdkconfig.defaults` ani logowane. Dobór metody injekcji sekretów musi uwzględnić, że firmware może być wyciągnięte z urządzenia — wspólne hasło nie stanowi silnego zabezpieczenia.
- Lokalny panel konfiguracji AP/LAN dostępny bez osobnego uwierzytelniania na jawne polecenie użytkownika (ADR0032). Dostęp sieciowy pozwala zmieniać ustawienia. Zachowaj Origin/custom header i walidację; politykę przyszłej aplikacji ustal osobno.
- TLS w HTTP streams z weryfikacją certyfikatu; sprawdź synchronizację czasu.
- OTA: wyłączone na polecenie użytkownika (ADR0055). Aktualizacje przewodowe, jedna factory, bez otadata/slotów rollback. Secure Boot/Flash Encryption nie są włączone.

## Testy
NVS flash wear, restarty w trakcie importu/OTA, niskie zasoby, weryfikacja kontenera, brak hasła w serial log, zachowanie na utratę zasilania.

## Pierwszy zakres implementacyjny — preferencje UI 05d

Komponent preferences zapisuje wyłącznie volume/wizualizację w wersjonowanym
blobie NVS. Polityka debounce i formatu testowana na hoście, flash wykonuje
osobny worker; odczyt jest częścią startup. [ADR 0017](adr/0017-deferred-ui-preferences.md).
Brak klucza daje domyślne 50/spectrum. Błędny lub nieobsługiwany rekord
wyłącza serwis bez automatycznego kasowania. Pozostałe klucze/sekrety,
LittleFS, OTA i kontrolowany shutdown/flush nie są zaimplementowane.

06b: UI obserwuje volume_revision właściciela audio i może zgłosić
submit_changed nawet przy tej samej wartości końcowej po sklejeniu zmian.
Zachowano jednego producenta skrzynki NVS i zapis po5s od obserwacji zmiany.

09a: prywatny AP JSON poza repo, getpass i uprawnienia0600. W firmware klucz
build-time, nie secret Kconfig ani zapis credentials do NVS. Brak klucza
nie otwiera AP. Nowa factory1500KiB przy tym samym NVS/PHY i offsetach;
nie kasować NVS. Portal/STA/admin auth, OTA i szyfrowanie nadal OPEN.

09b: użytkownik zezwolił na wyświetlanie stałego hasła AP na OLED podczas AP.
Prywatny build/stałość klucza bez zmian, brak haseł w repo/logach/HTTP DTO.
Nie traktować tej decyzji jako wyboru osobnego auth administracji. Publiczny
portal jest wyłącznie do odczytu; brak zapisu Wi-Fi/NVS i nowych endpointów
sterowania. Wyłączenie AP nastąpi po udanym STA/IP w kolejnym zakresie.

Uzupełnienie: zapamiętywana ma być lista profili Wi-Fi w kolejności priorytetu,
nie wyłącznie jeden zestaw credentials. Wersjonowany zapis/rollback i limity
pozostają do implementacji. Katalog stacji z uploadu playlist.csv trafia docelowo
do LittleFS dopiero po walidacji/zatwierdzeniu; prywatny plik workspace jest
wykluczony z Git i nie ma być zasobem wbudowanym. ADR0030.

ADR0047: strefa zegara jako string NVS streamoo_clock/zone, defaultWarsaw,
cache odświeżany dopiero po commit. Odczyt raz przy starcie, bez erase
przy błędzie. Flash wyłącznie poza callbackiem I2S. Ponowny zapis tej
samej utrwalonej strefy pomijany. Dane Wi-Fi/playlisty bez zmian.

ADR0054/06k: factory powiększona0x177000→0x180000 wyłącznie do istniejącej
pustej luki; offset0x10000 bez zmian. LittleFS nadal0x190000/0x100000,
NVS/PHY bez zmian. Pełny flash obejmuje nową tablicę, bez erase/migracji.
Nie jest to docelowy układ OTA; dodatkowe sloty pozostają OPEN.

ADR0055 zastępuje poprzedni układ06k: użytkownik rezygnuje z OTA i zgadza
się ponownie zaimportować stacje. Factory0x10000/0xEF0000, storage
0xF00000/0x100000. NVS/PHY zachowane; marker init per adres partycji
sfs_00f00000/init. Nowy blank storage przygotuje się mimo starego markera
w NVS; niepusty uszkodzony FS nadal bez autoformatowania. Instrukcja
LARGE_FLASH_BRINGUP.md. Historyczne plany OTA powyżej są zastąpione.
