# ADR 0010 — niezależne widżety wizualizacji

Data: 2026-10-24. Status: 04g programowo zweryfikowane;
nowe widżety na OLED HARDWARE_TEST_PENDING.

## Odbiór poprzedniego etapu

Po wersji 04f użytkownik potwierdził prawidłowe działanie. Zapisano odbiór
funkcjonalny przewijania wyłącznie w lewo według kontekstu commita 30df2e2.
Nie otrzymano oddzielnego hasha urządzenia ani logów pomiarowych. Pozostaje
wcześniejsza decyzja, że wygląd czcionki jest tymczasowy i będzie dopracowany.

## Interfejs i odpowiedzialność

Oryginalny komponent C11 `ui_widgets` zależy tylko od `graphics`. Jego API
otrzymuje canvas, bounds, rodzaj widżetu i niemodyfikowany snapshot modelu.
Nie zna SSD1322, sieci, odtwarzacza, RTOS, dekodera ani DSP. Nie alokuje,
nie liczy FFT, nie stosuje volume i nie zapisuje NVS. Nowych bibliotek
zewnętrznych nie dodano; standardowe API C i graphics sprawdzono przez
kompilację hostową oraz ESP-IDF 6.1. Nie skopiowano implementacji ani grafiki
z yoRadio ani innych bibliotek.

Model zawiera normalized display levels 0..32767 dla RMS/peak L/R i do
64 gotowych pasm widma. To jawna skala prezentacji, nie kontrakt próbek PCM,
dBFS ani kalibracja VU. Producent danych w późniejszym DSP odpowiada za
analizę przed volume, skalowanie, pasma logarytmiczne, smoothing i peak hold.
Widżet renderuje już przygotowane wartości; dane nie mogą być równolegle
modyfikowane podczas wywołania ani nakładać się na framebuffer.

Trzy wymienne funkcje za wspólnym API:

- spectrum: pionowe pasma z odstępem i linią podstawy, obsługa 0..64 pasm;
- stereo bargraph: L na górze, R na dole, poziom RMS i osobny marker peak;
- analog: L po lewej i R po prawej, dwa łuki i wskazówki, marker peak na łuku.

Wskazówki używają własnej tabeli 9 kierunków i interpolacji integer. Nie
wywołują trygonometrii, float ani FFT. Jest to geometria wizualna; nie
przedstawiamy jej jako skalibrowanej skali miernika VU.

Bounds muszą leżeć na powierzchni i mieć minimum 8×8; domyślnie są to
x44/y32/132×32 zgodnie z proponowanym układem dokumentacji. Widget dostaje
canvas potomny i czyści tylko własny obszar, respektując również clip rodzica.
Niepoprawny rodzaj/model/zakres nie zapisuje żadnych pikseli. Zmiana rodzaju
nie pozostawia pikseli poprzedniego widżetu. Wygląd na małych bounds może
wymagać strojenia, ale granice pamięci są przestrzegane.

## Diagnostyka i ograniczenia

W istniejącej 20-sekundowej animacji aplikacji tryby zmieniają się co 6 s:
spectrum, bargraph, analog. Boki mają etykiety TEST i FFT/L/R/VU, a snapshot
jest wytwarzany wyłącznie przez `main/visualization_probe.c` z syntetycznego
przebiegu trójkątnego. Nie jest to odtwarzanie audio ani wykonana analiza FFT.
Nagłówek i scroll pozostają; całkowity test nadal około 38 s i OFF.
Nie dodano obsługi gestu wyboru trybu, aktywnego UI managera ani playera.

Testy: oba kanały, silence/full-scale, maksymalna liczba pasm, przełączanie,
zmieniane bounds, inherited clip, błędne snapshoty, canary i niemodyfikowany
model. Zaktualizowany wzorzec całego ekranu obejrzano na hoście przed zapisem.
ASan/UBSan obejmuje małe, nieparzyste i pełnoekranowe bounds oraz poziomy.
LeakSanitizer nadal nie działa pod ptrace, detect_leaks=0, bez deklaracji
testu wycieków. Odbiór widżetów i ich czytelności na PCB jest osobnym testem.
