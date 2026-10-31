# ADR0028 — konfiguracja AP bez odtwarzania

Data:2026-10-31. Status: wdrożone; odbiór nowej polityki na S3 OPEN.

## Decyzja użytkownika

W trybie AP nie ma dostępu do streamu, więc odtwarzanie nie ma sensu.
MicroSD może być przyszłym lokalnym źródłem, ale nie jest obecnie
zaimplementowane. Dotychczasowe AAC w AP było eksperymentem współpracy
RF/HTTP z audio; użytkownik potwierdził czysty dźwięk. Kończymy ten domyślny
tryb testowy w AP i przechodzimy do docelowego zachowania konfiguracji.

## Implementacja

Network udostępnia jawną, bez-I/O decyzję bootstrap: boot_ap_enabled, prawda
przy aktywnym profilu AP i poprawnym wbudowanym kluczu. Aplikacja sprawdza
ją przed uruchomieniem audio. Dla konfiguracji AP nie tworzy audio_control,
dekodera, fixture_reader, tap task ani kanału I2S; nie uruchamia też starego
skończonego smoke tone. Nie jest to samo wyciszenie gainem. BSS istniejących
modułów może nadal zajmować miejsce w obrazie/RAM; nie deklarujemy usunięcia
wszystkich statycznych buforów. I2S sample callback i istniejący pipeline bez zmian.

Enkoder na ekranie AP nie steruje ukrytym playerem ani preferencjami. Portal
pokazuje audio.available=false/running=false i „Audio wyłączone w trybie AP”,
bez pozornego odtwarzania/volume. SSID/stały klucz na OLED i HTTP pozostają.
Gdy profil AP wyłączony albo brakuje klucza (radio off), dotychczasowe testy
audio działają jak wcześniej. Awaria startu AP nie uruchamia tonu zastępczego.
Klucz i NVS volume/widżetu nie są modyfikowane.

To polityka obecnego startu AP, nie zaimplementowane dynamiczne przełączenie
playera STA→AP. Przy przyszłym STA/rollback potrzebny będzie kontrolowany
STOP/fade/teardown ownera i wybór źródła. MicroSD osobno OPEN, bez domyślania
pinów, sterownika, filesystemu i nowych bibliotek. AP nadal ma się wyłączyć
po udanym STA z IP; STA pozostaje następnym zakresem.

## Weryfikacja

202 testy hostowe PASS. Pięć nowych integruje rzeczywistą treść main.c ze
stubami peryferiów: brak obu wariantów audio w AP, brak tonu po błędzie sieci,
zachowanie sterowanego audio i finite tone przy wyłączonym AP. Frontend DOM
sprawdza komunikat wyłączenia i brak wyświetlania aktywnej głośności.
Buildy w STATUS. Bez nowych zależności/licencji ani alokacji/I/O w ISR.

Odbiór: komunikat Audio disabled, brak fixture_reader/codec_fixture/I2S
logów, cisza mimo gestów, działające OLED SSID/klucz/URL oraz portal statusu.
Nie flashowano za użytkownika. Rzeczywista cisza DAC, wyświetlacz i portal
w tej konfiguracji pozostają do potwierdzenia.
