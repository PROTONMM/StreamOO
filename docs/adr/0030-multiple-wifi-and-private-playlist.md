# ADR0030 — kilka sieci Wi-Fi i prywatna playlista

Data:2026-10-31. Status: wymagania ustalone, format sprawdzony; integracja WWW/STA/import OPEN.

## Decyzje użytkownika

Konfiguracja przez WWW pozwala zapisać kilka sieci Wi-Fi. StreamOO łączy się
z pierwszą dostępną. Kolejność zapisanej listy przyjmujemy jako priorytet,
nie siłę RSSI. Próba z błędnym hasłem lub bez uzyskania IP nie może blokować
wszystkich dalszych dostępnych profili. AP wyłączamy dopiero po udanym STA/IP.
Hasła STA są konfigurowalne i trwałe; nie mylić ich ze stałym kluczem AP.

Przez WWW ma być dostępny upload playlist.csv ze stacjami. Prywatny przykład
użytkownika znajduje się w głównym katalogu workspace. Dodano /playlist.csv
do .gitignore i sprawdzono, że nie jest śledzony. Nie kopiujemy go ani jego
nazw/URL do fixture, dokumentacji lub firmware build. Oryginał pozostaje bez zmian.

## Weryfikacja formatu

Lokalny plik: UTF-8, bez BOM,267 rekordów, po3 pola rozdzielone TAB, zakończenia
LF, bez ostatniego LF. Semikolony są częścią pól, nie separatorem. Gramatyka
zgodna z literalnym podziałem name/TAB/url/TAB/ovol w referencji yoRadio:
https://github.com/e2002/yoradio/blob/main/yoRadio/src/core/config.cpp#L769
Sprawdzono2026-10-31. GPLv3 upstream sprawdzone w LICENSE; wykorzystujemy
wyłącznie informację o formacie, bez kopiowania implementacji lub API.

Trzecie pole zachowujemy jako oryginalne ovol; w pliku wartości0..63.
Nie utożsamiamy go automatycznie z naszym gain0..100 ani wartością dB.
11 URL ma niewidoczny U+200B przed schematem,3 URL się powtarzają.
Wymagają komunikatu w podglądzie; nie wolno po cichu usuwać/reorganizować
stacji ani nadpisywać prywatnego oryginału. Reguły normalizacji i korekcji
głośności trzeba ustalić przed zatwierdzaniem importu.

## Dostarczony eksperyment

Własny read-only tools/check_playlist.py weryfikuje format, UTF-8, kontrolne/
niewidoczne znaki, HTTP(S) URL i całe pole liczbowe. Raport zawiera wyłącznie
numery wierszy/kody błędów/liczniki, bez nazw, URL i credentials. Duplikaty
są ostrzeżeniem, nie automatycznym usunięciem. Nie łączy się z adresami stacji.
Na prywatnym pliku:267/256 poprawnych/11 niepoprawnych,3 ostrzeżenia duplikatów;
sprawdzono niezmieniony hash przed/po bez zapisywania jego wartości do repo.

Limity narzędzia:256KiB,2048 wierszy, nazwa128B, URL2048B, ovol signed32.
To granice eksperymentu walidacji, nie ustalony kontrakt pojemności urządzenia.
211 testów hostowych PASS,9 nowych z wyłącznie syntetycznymi danymi; parser/
CLI nie zapisuje pliku wejściowego i nie wypisuje jego treści. Firmware bez zmian,
nie powtarzano native buildów ani flashowania.

## Kolejność integracji

Najpierw zabezpieczony zapis uporządkowanych profili i worker STA z timeoutami,
przechodzeniem do dalszych profili i rollback. Wersjonowany rekord profili w NVS,
bez haseł w repo/logach/odpowiedziach GET. Admin auth D18 nadal OPEN; nowe
wymagania nie uchylają tego warunku przed dodaniem operacji modyfikujących.
Liczba profili, obsługa ukrytych SSID i zachowanie roamingu wymagają doprecyzowania.
Nie wybieramy nowej technologii składowania lub biblioteki bez przeglądu API/licencji.

Następnie upload → walidacja → podgląd problemów → zatwierdzenie → atomowa
wymiana katalogu w LittleFS. Stable ID niezależne od numeru wiersza, bez
zatrzymywania grającej stacji podczas samego uploadu/podglądu. Przerwany/błędny
import zachowuje poprzednią listę. Nowy kontrakt API StreamOO, bez kopiowania
endpointów yoRadio. Endpointy i trwały zapis nie są jeszcze zaimplementowane.
