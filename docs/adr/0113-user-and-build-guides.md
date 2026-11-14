# ADR0113 — instrukcje obsługi, menuconfig i budowania

Data: 2026-11-14. Status: wdrożone na polecenie użytkownika.

## Powód i decyzja

README łączyło aktualne radio z dawnymi etapami testów bez sieci i audio.
Użytkownik poprosił o aktualizację dokumentacji, instrukcję obsługi oraz
konfigurowania przez menuconfig i budowania. Powstały dwa dokumenty:
[USER_GUIDE.md](../USER_GUIDE.md) i [BUILD_GUIDE.md](../BUILD_GUIDE.md).
README jest teraz krótkim punktem wejścia; zachowuje sekcję MIT z ADR0112.
Historia rozwoju pozostaje w etapach, STATUS i ADR, z odsyłaczami do instrukcji.

Instrukcja obsługi opisuje AP/STA, enkoder A/B, cztery widoki, motywy WWW,
sterowanie i głośność, format/bitrate, edytor, dopisujący import, eksport,
zapamiętywanie, formaty i diagnostykę. Rozróżnia listę odtwarzacza bez stron
od edytora po 25 pozycji oraz położenie importu w LAN i AP.

Instrukcja budowania rozdziela Kconfig, ustawienia runtime i prywatny klucz
AP. Podaje rzeczywiste nazwy menu, symbole, GPIO 5/6/7 i różnicę względem
historycznego profilu B=16, konfigurację N16R8, dekoder, bufor i FFT.
Obejmuje pierwszą konfigurację, pierwszeństwo sdkconfig, build, flash,
partycje, zachowanie danych i osobne profile diagnostyczne.

## Źródła i weryfikacja

Treść zestawiono z main/Kconfig.projbuild, sdkconfig.defaults, CMake,
walidatorem, skryptem prywatnego AP, kodem UI/input/settings, frontendem,
formatem katalogu oraz istniejącymi protokołami. Polecenia idf.py i
instalację sprawdzono w oficjalnej dokumentacji ESP-IDF 6.1 podlinkowanej
w instrukcji. Nie przedstawiono historycznych prób jako nowego odbioru.

- Walidator GPIO/platformy: PASS.
- Natywny build ESP-IDF 6.1 / esp32s3: PASS.
- `python3 -m unittest discover -s tests -q`: 503 testy, PASS, bez SKIP.
- Lokalne odsyłacze nowych instrukcji, symbole Kconfig i składnia bloków
  poleceń powłoki: sprawdzone; `git diff --check`: PASS.

Nie zmieniono kodu, sdkconfig, partycji ani sekretów. Nie flashowano
urządzenia. Nie powtarzano testów przeglądarkowych ani odsłuchu na sprzęcie;
opisane komendy tych prób są instrukcją do wykonania, a nie nowym wynikiem.
