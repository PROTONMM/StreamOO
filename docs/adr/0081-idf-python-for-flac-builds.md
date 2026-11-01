# ADR0081 — interpreter ESP-IDF dla skryptu porównawczego

Data:2026-11-01. Status: host/build PASS; flash pozostaje po stronie użytkownika.

Użytkownik uruchomił build_flac.py z python3, który wskazywał /usr/bin/python3.
Skrypt przekazywał sys.executable do idf.py, więc pomijał środowisko Python
SDK i kończył się No module named rich_click. Sam napis (venv) w terminalu
nie potwierdza interpretera użytego w procesie potomnym. To błąd wrappera,
nie dowód, że należy instalować zależności IDF do innego venv lub globalnie.

## Decyzja

Wybór Pythona niezależnie od interpretera uruchamiającego wrapper:

1. IDF_PYTHON_ENV_PATH/bin/python, jeśli zmienna ustawiona; brak pliku daje
   jasny błąd zamiast cichego powrotu do systemowego Pythona.
2. Bez tej zmiennej odczyt wersji z tools/cmake/version.cmake danego SDK,
   canonical python_env/idf{major.minor}_py{major.minor Pythona}_env pod
   IDF_TOOLS_PATH lub ~/.espressif. Najpierw zgodny Python, potem jedyny
   dostępny interpreter tego SDK. Inna wersja IDF nie jest wybierana.

Konwencja sprawdzona w get_python_env_path() przypiętego idf_tools.py6.1.
Nie rozwiązujemy symlinku venv/bin/python do /usr/bin/python3: Python musi
wystartować przez ścieżkę venv, aby odnaleźć jego pyvenv.cfg i pakiety.
W środowisku potomnym ustawione spójne IDF_PYTHON_ENV_PATH, VIRTUAL_ENV,
PYTHON oraz prefiks PATH. Build i flash używają tego samego interpretera.
Nie zmieniamy środowiska powłoki rodzica ani zainstalowanych pakietów.

IDF nie pozwala użyć istniejącego katalogu CMake skonfigurowanego innym
Pythonem. Wrapper odczytuje PYTHON z CMakeCache.txt; przy zmianie zachowuje
stary katalog jako flac-dr-previous-python-N (analogicznie inne backendy)
i buduje w świeżym katalogu. SDKCONFIG poza build/ pozostaje zachowany.
Unikamy idf.py fullclean: hook component managera6.1 może usuwać wspólny
managed_components. Kopia zachowuje ELF/logi, a zmiana dotyczy tylko wariantu.

Nadal wymagane aktywne narzędzia SDK6.1, nie tylko pakiety Python: np.
ESP_IDF_VERSION i toolchain PATH po export.sh. Lokalny eksperyment z samym
IDF_PATH/Python bez eksportu SDK wykazał błąd inicjalizacji component managera;
wrapper nie zastępuje pełnego export.sh ani nie instaluje narzędzi SDK.

## Weryfikacja

418 testów hostowych PASS, w tym 9 testów wrappera: explicit venv, zachowanie
ścieżki symlinku, wykrycie wersji SDK, brak cichego fallbacku, zgodny wybór
Pythona w build/flash, archiwizacja starego CMake i zachowanie źródeł/config.
Rzeczywisty wrapper uruchomiony przez /usr/bin/python3 z obcym VIRTUAL_ENV
i eksportem SDK6.1 wybrał
~/.espressif/python_env/idf6.1_py3.12_env/bin/python, a następnie zbudował
i zweryfikował dr_flac. Nie instalowano pakietów do SDK/globalnego Pythona.
Zmiana nie dotyka firmware audio, buforów, NVS, playlist ani GPIO.
Agent nie wykonywał flash/erase/eFuse.
