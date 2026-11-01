# Przegląd możliwości wydania na MIT — 2026-11-01

Szczegółowa analiza wymian wszystkich modułów: [MIT_MIGRATION_ANALYSIS](MIT_MIGRATION_ANALYSIS.md).
Pełny inwentarz bieżącego obrazu i zasobów: [CSV](MIT_MODULE_INVENTORY.csv).

## Wniosek

**Własny kod aplikacji StreamOO może otrzymać MIT, jeżeli wydający ma prawa
niezbędne do udzielenia tej licencji. Obecnego kompletnego firmware i całego
repozytorium nie można przedstawiać jako objętych wyłącznie zwykłą MIT.**
To ocena zestawu plików i deklarowanych licencji, bez nadania nowej licencji.
Nie dodano głównego LICENSE ani nie założono tożsamości właściciela praw.

Dwa istotne punkty:

1. Mała czcionka PL ma nieustalone prawa do modyfikacji bitmap. To blokada
   publicznego wydania tej zawartości do czasu uzyskania zgody albo zastąpienia
   jej fontem o ustalonych warunkach. Polecenie użytkownika zezwalające na
   tymczasowe skopiowanie nie zastępuje licencji autora bitmap.
2. `espressif/esp_audio_codec` 2.6.2 ma **Espressif Modified MIT**, ograniczoną
   do produktów Espressif. Użycie w StreamOO na ESP32-S3 odpowiada temu
   ograniczeniu; nie wolno usuwać go i obiecywać odbiorcom swobodnego użycia
   tego komponentu na innych platformach. Wybór dr_flac nie usuwa biblioteki:
   pozostałe kodeki i parsery kontenerów nadal z niej korzystają.

## Materiały i zakres

Sprawdzono manifesty/lock, CMake dekodera, lokalne teksty w `licenses/`,
`THIRD_PARTY_NOTICES.md`, pochodzenie fontów i `third_party/ORIGINS.json`.
Wersje bez zmian: ESP-IDF6.1, audio_codec2.6.2, ESP-DSP1.8.2,
cJSON1.7.19~2, mDNS1.14.0, LittleFS adapter1.20.4, dr_flac0.13.4,
micro-flac0.3.0. To zestawienie najważniejszych zależności aplikacji;
ESP-IDF zawiera również własne zależności i pliki z odrębnymi notami.
Pełne archiwum dystrybucyjne musi zachować ich źródłowe oznaczenia.
Nie przeprowadzono audytu praw patentowych ani niezależnego audytu wnętrza
zamkniętych bibliotek binarnych producenta.

| Materiał | Warunki | Skutek dla wydania |
|---|---|---|
| Własne adaptery, sterowniki, UI, DSP, player, REST i narzędzia | Licencja projektu dotąd nienadana | Możliwe MIT po ustaleniu uprawnionego podmiotu i zakresu |
| Espressif audio_codec2.6.2 | LicenseRef-Espressif-Modified-MIT | Zachować tekst i atrybucje; tylko produkty Espressif, również dla binarki |
| ESP-IDF / ESP-DSP / mDNS / micro-flac | Apache-2.0 w opisanym zakresie | Zachować licencje, wymagane NOTICE i oznaczenia zmian; kod pozostaje na swoich warunkach |
| Mbed TLS4.1.0 w SDK | Apache-2.0 OR GPL-2.0-or-later | Wybrano Apache-2.0; sama obecność drugiej opcji nie oznacza wyboru GPL |
| dr_flac0.13.4 | MIT-0, wybrana alternatywa upstream | Nie ogranicza MIT własnego adaptera; zachowano plik i pochodzenie |
| cJSON / HTTP parser / LittleFS adapter | MIT | Zachować odpowiednie copyright i teksty licencji |
| LittleFS core; noty FLAC/Vorbis/Opus producenta | BSD i dodatkowe noty upstream | Zachować noty; nie przypisywać im licencji własnej aplikacji |
| Monaspace Krypton / przekonwertowany StreamOO UI16 | OFL-1.1 | Font i jego podzbiór zachowują OFL; aplikacja może mieć MIT; zachować nazwę pochodnej i notę |
| Mały font `streamoo_font5x8.c` | Baza Adafruit BSD; modyfikacje PL UNKNOWN | Wyjaśnić prawa lub wymienić bitmapy; dotyczy też obrazów referencyjnych zawierających te kształty |
| Mozilla CA w bundlu TLS | MPL-2.0 | Dołączyć tekst, notę i dostęp do odpowiadającego źródła CA, już zachowanego w `licenses/sources/cacrt_all.pem` |
| `references/`, oryginalne materiały yoRadio/schemat | Materiał odniesienia, nie target | Nie obejmować automatycznie MIT projektu; osobno zweryfikować przed publikacją archiwum tych materiałów |
| Prywatne playlisty, hasła, konfiguracje i artefakty lokalne | Nie są częścią repozytorium do wydania | Nie dołączać do paczki źródeł/firmware publicznego |

Nie znaleziono nowego importu implementacji GPL do docelowego firmware.
Uruchamianie FFmpeg/ffprobe jako osobnych narzędzi hostowych do oryginalnych
sygnałów testowych nie oznacza linkowania ich z firmware. Nie jest to
stwierdzenie, że każdy materiał referencyjny w repo ma licencję MIT.

## Jak przygotować wydanie

- Rozstrzygnąć pochodzenie małego fontu PL albo zastąpić jego bitmapy
  i odpowiednio zregenerować testy obrazów. Nie usunięto polskich liter
  z działającego radia w ramach tego przeglądu.
- Ustalić właściciela praw do własnych plików i nadać im MIT z jednoznacznymi
  wyjątkami dla danych fontów, `third_party/`, zależności SDK, `licenses/`
  oraz materiałów referencyjnych. Sam napis MIT w README nie zmienia tych praw.
- Paczka binarna: dołączyć THIRD_PARTY_NOTICES, właściwe teksty licencji
  i wymagane NOTICE oraz źródło CA/MPL. Odbiorca musi móc zapoznać się
  z ograniczeniem Espressif; nie trzeba wyświetlać pełnych licencji na OLED.
- Paczka źródłowa: zachować manifesty, wersje, hashe i noty; wykluczyć
  prywatne katalogi, sekrety, playlisty i niezweryfikowane materiały referencyjne.
- Jeżeli celem jest firmware przenośny bez ograniczenia do Espressif,
  trzeba również zastąpić objęte nim kodeki/parsery. Zamiana samego FLAC
  tego nie zapewnia. Nie jest to warunek użycia MIT dla własnych plików.

## Źródła pierwotne sprawdzone przy przeglądzie

- [Standardowa MIT — OSI](https://opensource.org/license/mit): prawa i obowiązek zachowania noty.
- [Dokładna licencja audio_codec2.6.2](https://components.espressif.com/components/espressif/esp_audio_codec/versions/2.6.2/license): ograniczenie do Espressif; lokalna kopia w `licenses/`.
- [Apache License2.0, punkt4](https://www.apache.org/licenses/LICENSE-2.0): warunki redystrybucji i NOTICE.
- [SIL OFL FAQ, sekcja1](https://openfontlicense.org/ofl-faq/): fonty w pakiecie z aplikacją i odrębne warunki fontu.
- [Mozilla MPL2.0 FAQ](https://www.mozilla.org/en-US/MPL/2.0/FAQ/): zakres plikowy i dystrybucja źródeł pokrytych MPL.
- Pinned źródła dr_flac/micro-flac i hashe: `components/decoder/third_party/ORIGINS.json` oraz oryginalne nagłówki/LICENSE.
- Dokładny hash źródła bitmap PL, zgoda użytkownika i stan UNKNOWN: `THIRD_PARTY_NOTICES.md`, ADR0006.

## Uzupełnienie: runtime toolchainu

Pełniejszy przegląd mapy wykazał również libgcc/libstdc++ (GCC Runtime
Library Exception3.1) i Picolibc/libc/libm z warunkami plikowymi. Wyjątek
GCC pozwala na niezależne aplikacje nie-GPL przy spełnieniu jego warunków;
nie należy utożsamiać go ze zwykłym linkowaniem biblioteki GPL bez wyjątku.
Wcześniejsze stwierdzenia o braku importu GPL dotyczą integracji
implementacji kodeków, nie są stwierdzeniem jednorodnej licencji całego
runtime. Szczegóły i źródła pierwotne w MIT_MIGRATION_ANALYSIS.
