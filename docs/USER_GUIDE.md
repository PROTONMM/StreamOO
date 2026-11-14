# StreamOO — instrukcja obsługi

[English version](en/USER_GUIDE.md)

Stan instrukcji: 2026-11-14. Dotyczy zwykłego profilu radia, z włączoną
siecią, ekranem i enkoderem. Budowanie i wgrywanie opisuje
[BUILD_GUIDE.md](BUILD_GUIDE.md). Historia testów i otwarte kwestie:
[STATUS.md](STATUS.md).

## Pierwsze uruchomienie

1. Włącz radio. Logo StreamOO pojawia się przy starcie i może pozostawać
   widoczne podczas łączenia z siecią oraz rozpoczynania odtwarzania.
2. Bez zapisanych sieci radio uruchomi punkt dostępu **StreamOO-XXXXXX**.
   OLED pokaże nazwę sieci, hasło i adres strony konfiguracji. Hasło pochodzi
   z prywatnego procesu budowania; nie istnieje publiczne hasło domyślne.
3. Połącz komputer lub telefon z tą siecią. Na Androidzie zaakceptuj
   pozostanie w sieci bez Internetu. W razie automatycznego przełączania
   połączenia wyłącz na czas konfiguracji transmisję komórkową.
4. Otwórz ręcznie **http://192.168.4.1/**. Automatyczne otwarcie portalu
   nie jest wymagane ani gwarantowane. Strona działa lokalnie, z JavaScriptem.
5. W **Sieci Wi-Fi** dodaj nazwę domowej sieci 2,4 GHz i jej hasło,
   następnie wybierz **Zapisz i połącz**. Dla sieci bez hasła zaznacz
   **Sieć otwarta, bez hasła**.
6. Po połączeniu radio zapisze profile i po około 5 sekundach wyłączy AP.
   Przełącz telefon/komputer do tej samej sieci domowej. Otwórz adres IP
   podany na OLED lub link pokazany przez portal przed rozłączeniem AP.
7. Dodaj stację w zakładce **Playlista** albo zaimportuj plik przez
   **Playlista → Import playlisty**. Kliknij stację w **Odtwarzaczu**.

Podczas konfiguracji AP audio jest wyłączone. Można wtedy ustawić Wi-Fi,
zaimportować stacje i wybrać strefę czasu; odtwarzacz i edytor są dostępne
po przejściu do sieci domowej. Import można wykonać jeszcze przed krokiem 5.

Przy kolejnych startach radio najpierw próbuje zapisanych sieci w kolejności
priorytetu, do około 15 sekund na profil. AP pojawia się dopiero po
niepowodzeniu wszystkich prób. Domyślnie uruchamiana jest ostatnia zapisana
stacja, jeśli nadal istnieje. Zachowanie startu można zmienić w ustawieniach.

## Dostęp do panelu WWW

W sieci domowej użyj `http://ADRES_IP_RADIA/`. Alternatywnie działa nazwa
`http://streamoo-xxxxxx.local/` albo własna nazwa ustawiona w panelu.
Jeśli telefon lub router nie obsługuje poprawnie mDNS, użyj adresu IP.
Po zmianie sieci/adresu DHCP poprzedni adres może być nieaktualny.

Panel jest lokalny i nie wymaga dodatkowego logowania. Osoby mające do
niego dostęp sieciowy mogą sterować radiem i zmieniać ustawienia;
nie wystawiaj go przez przekierowanie portów do Internetu. Po aktualizacji
firmware odśwież stronę z pominięciem pamięci podręcznej, jeśli widzisz
stary wygląd lub niedziałające przyciski.

### Odtwarzacz

- Kliknięcie nazwy stacji rozpoczyna jej odtwarzanie.
- Lista zawiera wszystkie zapisane stacje w przewijanym oknie, bez stron.
  Pole **Szukaj stacji…** filtruje nazwy; **↻** odczytuje katalog ponownie.
- Przyciski ikonowe oznaczają kolejno: **poprzednia stacja, odtwarzaj,
  zatrzymaj, następna stacja**. Poprzednia/następna działa według całego
  katalogu, niezależnie od filtra, z przejściem między końcem a początkiem.
- Suwak reguluje głośność już podczas przesuwania. Zakres wynosi 0–100;
  0 wycisza, a 100 oznacza brak cyfrowego tłumienia.
- STOP kończy strumień i wygasza OLED; PLAY ponownie łączy wybraną stację.
  To nie pauza z zapamiętaniem fragmentu audycji. Panel WWW nadal działa.
- Widoczne są kodek, bitrate i częstotliwość próbkowania. **≈** oznacza
  oszacowanie bitrate na podstawie dekodowanego audio, **Zmienny** oznacza
  wykrytą zmienność, a **—** brak danych. Wykrywanie zmienności wymaga
  obserwacji przez kilka sekund; liczba bez tego oznaczenia nie dowodzi CBR.

Motyw **Systemowy / Jasny / Ciemny** wybierzesz w nagłówku. Jest zapamiętywany
w danej przeglądarce, osobno dla adresu strony; nie zmienia wyglądu OLED.

### Playlista: edycja i kopia zapasowa

1. W **Playlista** rozwiń **Dodaj stację**, wpisz nazwę i adres strumienia
   `http://` albo `https://`, a następnie **Dodaj do listy**.
2. Nazwy i adresy istniejących pozycji edytuj w ich polach. **×** usuwa
   wiersz z roboczej listy. Edytor pokazuje po 25 stacji na stronę.
3. Naciśnij **Zapisz zmiany**, aby zapisać cały edytowany katalog w radiu.
   **Odrzuć zmiany i odczytaj** przywraca zapisany stan.
4. **Eksportuj playlist.csv** pobiera zapisaną listę, bez niezapisanych
   poprawek edytora. Używaj eksportu jako kopii zapasowej.
5. **Usuń całą listę** wymaga potwierdzenia i usuwa zapisany katalog.
   Najpierw wykonaj eksport. Usunięcie aktywnej stacji zatrzymuje odtwarzanie.

Zmiana nazwy lub URL stacji zachowuje jej identyfikator. Grająca stacja nie
restartuje się po takiej edycji; nowy adres zostanie użyty przy następnym
połączeniu. Ponowny wybór tej samej już grającej pozycji też nie restartuje
strumienia. Przy konflikcie edycji z drugą przeglądarką odczytaj aktualną
listę i ponów poprawki; nie zakładaj, że odrzucony zapis został zastosowany.

### Import pliku

W sieci domowej otwórz **Playlista → Import playlisty**; w trybie AP
sekcja importu jest na stronie konfiguracji. Wybierz plik, naciśnij **Sprawdź
playlistę**, przeczytaj wynik i dopiero wtedy **Zatwierdź import**.
Import **dopisuje** stacje. Powtórzone adresy są raportowane i zachowywane;
ponowny import tego samego pliku może utworzyć kolejne kopie wpisów.
Limit to 2048 pozycji łącznie i 256 KiB pliku.

Aby zastąpić katalog: wyeksportuj kopię, sprawdź plik do importu, usuń listę
osobnym przyciskiem, następnie ponownie sprawdź i zatwierdź import. Samo
sprawdzenie pliku niczego nie zapisuje. Pusty import nie służy do kasowania.

Mimo rozszerzenia `.csv` format jest następujący:

```text
nazwa stacji<TAB>adres HTTP lub HTTPS<TAB>ovol
```

To UTF-8, bez nagłówka, dokładnie trzy pola rozdzielone **rzeczywistym
tabulatorem**, nie przecinkiem, średnikiem ani tekstem `<TAB>`. Każda stacja
zajmuje osobny wiersz. Nazwa może mieć do 128 bajtów UTF-8, adres do 2048
bajtów. Pole `ovol` to liczba całkowita zachowywana dla zgodności z yoRadio;
dla nowych stacji wpisz `0`. Nie służy obecnie do osobnej regulacji głośności
stacji w StreamOO. Najłatwiej uzyskać wzorzec przez eksport z radia.

Plik może zawierać jednocześnie HTTP i HTTPS. Import nie zamienia protokołu
i nie sprawdza, czy każda stacja rzeczywiście nadaje. Adres powinien wskazywać
strumień lub obsługiwaną playlistę, a nie stronę WWW stacji.
Lokalny `playlist.csv` w katalogu projektu nie jest automatycznie wbudowywany
ani wgrywany razem z firmware.

## Enkoder

Domyślny **tryb A**:

| Gest | Ekran główny | Lista stacji |
| --- | --- | --- |
| Obrót | Zmiana głośności | Przesuwanie zaznaczenia |
| Krótki klik | Podczas grania: otwarcie listy; po STOP: PLAY | Wybór środkowej, zaznaczonej stacji |
| Długi klik | STOP i wygaszenie OLED | STOP, wyjście z listy i wygaszenie OLED |
| Obrót z wciśniętym przyciskiem | Zmiana wizualizacji | Bez działania |

Bez wybranej stacji krótki klik otwiera listę. Długi klik to przytrzymanie
około 0,6 s lub dłużej; polecenie następuje po zwolnieniu przycisku.
Po obrocie z wciśniętym przyciskiem zwolnienie nie wywołuje dodatkowego kliknięcia.

Lista OLED zatrzymuje się na pierwszej i ostatniej pozycji. Podczas
przeglądania bieżąca stacja gra dalej. Domyślnie po **5 sekundach bezczynności**
radio wybiera zaznaczoną stację. W ustawieniach WWW można wybrać 1–120 s
oraz powrót bez zmiany zamiast automatycznego wyboru.

Opcjonalny **tryb B**, wybierany podczas budowania w `menuconfig`, zmienia
gesty na ekranie głównym: krótki klik przełącza PLAY/STOP, długi otwiera
listę. Na liście krótki zatwierdza, a długi wychodzi bez zmiany stacji.

## OLED i wizualizacje

Ekran pokazuje nazwę stacji, przewijane metadane audycji, IP, głośność,
informacje audio/Wi-Fi oraz datę i zegar. Metadane zależą od nadawcy;
brak tytułu nie musi oznaczać błędu odtwarzania.

Obrót z wciśniętym przyciskiem przełącza cztery widoki:

| Widok | Znaczenie |
| --- | --- |
| Widmo | Maksymalny poziom prążków FFT w każdym paśmie częstotliwości |
| Bargraph L/R | Bieżący poziom RMS kanałów i osobny znacznik szczytowy |
| Wskazówki L/R | Poziom RMS wskazówką i peak znacznikiem z dwóch pikseli |
| Widmo z peak hold | To samo widmo, z zatrzymywanym i powoli opadającym znacznikiem szczytu |

Oba widma mają zakres **−45…0 dBFS**, tę samą nieliniową charakterystykę
wysokości (gamma 2) i szachownicowe wypełnienie słupków. Czwarty widok nie
jest widmem RMS. Pomiar audio odbywa się przed regulacją głośności, więc
wyciszenie suwakiem nie zmniejsza wskazań przy nadal odtwarzanym strumieniu.

## Ustawienia i zapamiętywanie

| Miejsce w WWW | Co można ustawić |
| --- | --- |
| Sieci Wi-Fi | Do 8 profili, kolejność prób przyciskami ↑/↓, hasła i sieci otwarte |
| Data i czas | Strefa czasowa; domyślnie Polska, z automatyczną zmianą czasu |
| Odtwarzanie i obsługa | Czas/akcja listy OLED, start zatrzymany lub odtwarzanie ostatniej stacji, nazwa `.local` |

Zapisane hasła Wi-Fi nie są wyświetlane. Puste hasło istniejącego profilu
zachowuje poprzednią wartość; do sieci otwartej użyj checkboxa. Nowa lista
zastępuje starą po udanym połączeniu. Jeśli nowe profile nie działają,
radio próbuje wcześniejszych. Jawne zapisanie pustej listy usuwa profile
i pozostawia tryb konfiguracji AP.

Nazwę urządzenia podaj bez `http://` i bez `.local`: małe litery, cyfry
i łączniki, do 63 znaków, bez łącznika na początku/końcu. Puste pole
przywraca nazwę wyznaczoną z MAC.

Głośność, wizualizacja i ostatnia stacja są utrwalane po około 5 sekundach
bez zmian. Przed odłączeniem zasilania odczekaj ten czas. Formularze WWW
wymagają własnego przycisku zapisu i potwierdzenia powodzenia. Zwykła
aktualizacja firmware przy niezmienionym układzie partycji zachowuje dane.

## Formaty i ograniczenia

Implementacja obejmuje MP3, AAC/HE-AAC, natywny FLAC, Ogg Vorbis/Opus
oraz rozwiązywanie adresów M3U/PLS. Obsługiwany zakres wyjścia to mono/stereo
8–48 kHz; obsługa formatu nie gwarantuje każdego profilu i każdej stacji.
Nie deklarujemy obsługi wielokanałowego audio, Ogg-FLAC ani surowego Opus/RTP.

HLS obejmuje nieszyfrowane audio MP3/AAC i MPEG-TS w obsługiwanym podzbiorze.
AES/DRM, fMP4, BYTERANGE i LL-HLS nie są obsługiwane; zmiana discontinuity
po starcie kończy sesję i prowadzi do ponownej próby. Szczegóły i odbiór:
[HLS_BRINGUP.md](HLS_BRINGUP.md), [FLAC_BACKENDS_BRINGUP.md](FLAC_BACKENDS_BRINGUP.md).

HTTPS wymaga poprawnego czasu UTC uzyskanego przez SNTP. Radio weryfikuje
certyfikaty; nie przechodzi automatycznie z HTTPS na HTTP. Nie ma OTA,
timeshiftu, obsługi pilota RC5 ani odtwarzania z karty microSD w zwykłym
profilu. Długotrwałe próby i niektóre scenariusze sprzętowe pozostają
opisane jako OPEN w statusie projektu.

## Rozwiązywanie problemów

| Objaw | Co sprawdzić |
| --- | --- |
| Telefon opuszcza AP radia | Pozostań w sieci bez Internetu; na czas konfiguracji wyłącz automatyczne przełączanie na dane komórkowe |
| Nie otwiera się panel | Ta sama sieć, bieżący IP z OLED/routera, adres z `http://`, brak izolacji klientów; przy problemie z `.local` użyj IP |
| Radio nie uruchamia AP ani nie łączy Wi-Fi | Sprawdź, czy build miał prywatną konfigurację AP i `STREAMOO_NETWORK_ENABLED`; patrz instrukcja budowania |
| OLED jest ciemny, WWW działa | Radio może być w STOP; naciśnij PLAY lub krótko enkoder w trybie A |
| Jest lista, ale nie ma dźwięku | Wybierz stację, sprawdź głośność i stan odtwarzania; w AP audio jest wyłączone; porównaj inną znaną stację |
| Długie oczekiwanie na HTTPS lub brak zegara | Sprawdź Internet, DNS i dostęp do SNTP; sama zmiana strefy nie synchronizuje UTC |
| Przerwy lub ponawianie połączenia | Sprawdź zasięg, dostępność źródła i obsługiwany format; zbierz log diagnostyczny według instrukcji budowania |
| Podwójne stacje po imporcie | Import dopisuje i zachowuje duplikaty; usuń zbędne wpisy w edytorze i zapisz |
| Błąd formatu playlisty | UTF-8, trzy pola TAB, liczba w trzecim polu, limity pliku i katalogu |
| Utracona ostatnia zmiana po odłączeniu zasilania | Odczekaj minimum 5 s po zmianie głośności, widoku lub stacji; sprawdź komunikat zapisu formularza |

Nie zaczynaj diagnostyki od kasowania całej flash. Eksport playlisty
i zapis ustawień, które chcesz zachować, ułatwią odtworzenie konfiguracji.
