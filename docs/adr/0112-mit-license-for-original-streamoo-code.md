# ADR0112 — MIT dla własnego kodu StreamOO

Data: 2026-11-14. Status: wdrożone na polecenie użytkownika.

Użytkownik wybrał standardową MIT dla własnej aplikacji i wskazał podpis
właściciela praw: Mariusz Księżak. Główny LICENSE zawiera niezmieniony tekst
MIT i Copyright (c) 2026 Mariusz Księżak. Identyfikator SPDX: MIT.
Tekst porównano z MIT opublikowaną przez OSI:
https://opensource.org/license/mit.

LICENSING.md określa zakres: własny kod, dokumentacja, narzędzia, testy,
konfiguracja oraz oryginalne zasoby, z zachowaniem praw do danych/fontów,
kodu third_party, SDK, runtime i materiałów referencyjnych. Nie dopisano
warunków do standardowego tekstu MIT ani nie przepisano cudzych copyright.
W szczególności ograniczenie esp_audio_codec 2.6.2 do produktów Espressif
pozostaje. Fonty Krypton/OFL, małe bitmapy public domain i CA/MPL mają
odrębne warunki. Historyczne rewizje materiałów zewnętrznych bez zmian.

README otrzymało widoczną sekcję licencji. Uaktualniono THIRD_PARTY_NOTICES,
29 pozycji własnego kodu/grafiki w inwentarzu oraz stan bieżący w historycznych
analizach MIT. Dawnych analiz nie przepisano wstecz.

Nowy licenses/README.md indeksuje teksty. Dołączono 9 tekstów/not:
GCC GPLv3 wraz z Runtime Library Exception 3.1, Picolibc/Newlib,
przegląd praw SDK, FreeRTOS, lwIP, wpa_supplicant oraz wybraną MIT-0 dr_flac.
Źródło: lokalny SDK fff9895c82d744c7237be8847347bdd1b07c6643 oraz toolchain
esp-15.2.0_20251204 używany przez aktualny build; dr_flac z zachowanego
nagłówka 0.13.4. UPSTREAM_SOURCES.json zawiera pochodzenie i SHA-256.
Osiem kopii jest identycznych bajtowo; tekst dr_flac to dokładna wybrana
alternatywa z nagłówka bez delimitera komentarza C. Nie importowano kodu.

Pełne inwentarze SDK/Picolibc zawierają także opisy składników opcjonalnych
lub hostowych, więc sama obecność licencji w nich nie dowodzi linkowania
objętego nią kodu. Nie deklarujemy, że nadanie MIT aplikacji stanowi pełny
audyt każdej konfiguracji albo nadaje nowe prawa do references/.

Walidacja:

- Standardowy tekst MIT (porównanie treści grant/disclaimer), poprawny podpis PASS.
- SHA-256 wszystkich 9 tekstów i zgodność ze źródłami PASS.
- Hash-e zachowanych plików third_party nadal zgodne z ORIGINS.json PASS.
- Lokalne odsyłacze dokumentacji licencyjnej PASS; git diff --check PASS
  dla zmian śledzonych. Nowe kopie upstream zachowują oryginalne formatowanie.
- Native ESP-IDF 6.1/esp32s3 build PASS: 0x177400 B, 90% app wolne.

Nie zmieniono implementacji firmware/frontendu. Dla tej zmiany dokumentacji
nie uruchamiano ponownie testów funkcjonalnych; sprawdzono treść, pochodzenie,
zakres i odsyłacze. Bez flash i bez zmiany wersji zależności.
