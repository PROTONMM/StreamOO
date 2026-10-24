# Testy hostowe

Uruchom `python3 -m unittest discover -s tests -v`. Wymagane Python 3.9+
i kompilator C (`cc`), bez instalowania bibliotek Pythona. Testy są także
wykrywane przez pytest, jeśli jest dostępny.

Walidator GPIO: 5 testów, w tym 2332 konfiguracje porównujące wykonany kod
C firmware z walidatorem Pythona; pin RESET, wszystkie sygnały, zakresy,
kolizje, rezerwacje USB/UART i błędne dane. Biblioteka testowa powstaje
wyłącznie w katalogu tymczasowym.

`python3 tools/check_config.py sdkconfig` sprawdza lokalną konfigurację.
CMake uruchamia tę kontrolę automatycznie. Testy hostowe nie zastępują
`idf.py build` ani protokołu testów PCB.

Profil platformy: dodatkowe 3 testy (poprawne defaults, odrzucenie każdej
z 8 błędnych opcji i brakujących opcji). Łącznie 8 testów.
Kontrola pełnego profilu: `python3 tools/check_config.py sdkconfig --platform`.
Telemetryczne zadania i pomiary elektryczne wymagają PCB — patrz
`docs/HARDWARE_BRINGUP.md`; nie są symulowane testem hostowym.
