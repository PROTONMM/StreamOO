# 11 — Przechowywanie, OTA, bezpieczeństwo

## NVS (USTALONE)
- Wi-Fi credentials, wybrany wariant runtime jeśli dopuszczony, aktualna głośność, ostatnia stacja stable_id, wizualizacja, własna nazwa mDNS, stan startowy.
- Każda zmiana głośności lub wizualizacji zeruje timer; **zapis po 5 sekundach bez nowych zmian**, gdy różni się od ostatniego trwałego stanu. Flush podczas kontrolowanego restartu; przy nagłym zaniku zasilania można utracić ostatnie 5 s zmian.
- Wybór trybu enkodera A/B jest domyślnie build-time `menuconfig`, nie opcją dynamiczną.

## LittleFS
Lista stacji, ewentualnie WWW assets, tymczasowy staging importu. 16 MB flash pozwala przewidzieć partycje 2×OTA + filesystem, ale podział przestrzeni ustalać po zmierzeniu firmware i potrzeb komponentów. Zastosuj wersjonowanie formatu stacji i kopię/rollback.

## Sekrety i sieć
- Hasło AP stałe w produktach, ale NIE commitowane do `sdkconfig.defaults` ani logowane. Dobór metody injekcji sekretów musi uwzględnić, że firmware może być wyciągnięte z urządzenia — wspólne hasło nie stanowi silnego zabezpieczenia.
- Wybierz tryb admin authentication (hasło sesyjne lub tokeny) przed wystawieniem API w LAN; bez uwierzytelnienia nie wolno dopuścić zmian ustawień.
- TLS w HTTP streams z weryfikacją certyfikatu; sprawdź synchronizację czasu.
- OTA: dwie partycje app, walidacja podpisów/hash, rollback po nieudanym starcie; Secure Boot/Flash Encryption jako ewentualny profil produkcyjny, nie udawaj że są włączone na starcie.

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
