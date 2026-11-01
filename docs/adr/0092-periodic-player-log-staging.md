# ADR0092 — uzupełnianie PCM między liniami diagnostyki

Data: 2026-11-01. Status: regresja hostowa odtworzona i poprawiona;
odbiór nowego firmware na S3 OPEN.

## Wynik kontrolowanego LAN

Załącznik2cc3711b-3339-4e1f-822d-a85946e2d2a7, SHA256:
`79a8aa9a6e616d99c3b8d2cc54575a97f58563db04dc7c14b1bc04a0d09b8099`.
Firmware b2431e5/ELFf76a8ebd, dr_flac-0.13.4. Brak nowej deklaracji
odsłuchu. Log kończy się w361,278s, obejmuje publiczny live i3 stacje LAN.

| Stacja / generacja | Okno od connect do zmiany/końca logu | Underrun | Recover/Reject |
|---|---:|---:|---:|
| Publiczny TEST FLAC native lossless /4 | 49,83s | 10 | 0/0 |
| TEST LAN FLAC16 tones /6 | 55,08s | 0 | 0/0 |
| TEST LAN FLAC24 noise /8 | 77,89s | 0 | 0/0 |
| TEST LAN FLAC24 live headerless /10 | 164,69s | 1 | 0/0 |

LAN16 i zwykły LAN24 bez low, hard0, starts1. W headerless low1/hard0/
starts2. Nie są to pełne300s prób ani test wielogodzinny. Brak końców EOF
w tych oknach; zamknięcie poprzednich stacji było wyborem użytkownika.

W LAN24 i headerless body stabilnie około1,84Mb/s, read_max22,301/23,346ms,
wait_empty0. Większy dopływ niż w publicznym live (~1,4Mb/s w tym logu)
nie wywołuje ciągłych niedoborów. Kod dr i bootstrap obsługują te dane na S3;
nie ma dowodu ogólnego braku mocy CPU do FLAC. Nie oznacza to obsługi
każdego możliwego materiału FLAC ani rozstrzygnięcia między WAN/TLS/ICY.
CPU podczas ciężkiego headerless średnio około71,56/66,67%, co obejmuje
sieć, UI i metering; nie przypisywać całej wartości samemu dekoderowi.

Jedyny low LAN w341,958s: compressed257024B, PCM14, cache23576B,
pending0, wait_empty0, wait_busy98, RX age0. W341,798s zaczynał się
okresowy raport (PCM32), ostatnia jego linia w341,858s. Dotychczasowy
log_max79,923ms, codec_max32,279ms, block_max50,240ms. To zbieżność
czasowa przy pełnym źródle, a nie dowód, że w tym konkretnym momencie
każde historyczne maksimum wystąpiło jednocześnie. Zapas PCM160ms i
fade przy50ms wymagają obsługi producenta przed dłuższą przerwą.

Początkowe trzy nieudane TLS przed gen4 są osobnym problemem. Starsze
Recover z innych logów i źródło uszkodzonych bajtów nadal OPEN. Brak ich
w krótkim teście publicznym nie oznacza naprawy.

## Reprodukcja

Rozszerzono test prawdziwego aktora playera i rzeczywistego PCM/gain.
Źródło ma stale dostępne dane; przygotowanie bloku kosztuje1ms, każda
okresowa linia logu20ms. Wirtualny czas uruchamia prawdziwy fill co5ms
również wewnątrz decode/logowania, a nie dopiero podczas vTaskDelay.

Przed zmianą test kończy się niepowodzeniem sprawdzenia underflows0 /
hard0 / starts1: pakiet7 linii blokuje producenta na140ms. To kontrolowany
model opóźnienia UART/zadań, nie pomiar rzeczywistego kosztu każdej linii
S3 ani dowód wyłącznej przyczyny niedoboru z logu użytkownika.

## Zmiana

Właściciel playera wypisuje najwyżej jedną okresową linię na iterację:
status, decode perf, decode buffers, buffer interval, Delivery, RMS, FFT.
Przed każdą kolejną linią normalna iteracja obsługuje komendy i uzupełnia
PCM. Początek raportu nadal około5s; zakończenie rozłożone na kolejne
iteracje. Zmiana generacji kasuje pozostałe kroki poprzedniego raportu.
Nie tworzymy taska, kolejki, dodatkowych buforów ani alokacji.

Pomiary w różnych liniach są zbierane na bieżąco, nie stanowią jednego
atomowego snapshotu całego raportu. Buffer interval resetowany przy swojej
linii, a przepływ Delivery nadal liczony według rzeczywistego interwału.
Dotychczasowe log_max_us całego pakietu zastąpiono log_step_max_us:
największy czas ścienny pojedynczego okresowego kroku w tej sesji.
Nie porównywać tych dwóch metryk jako speedupu. Format błędów i raport
kończącej się sesji zachowują szczegóły; fade jest zadany przed ich zapisem.

Jedno wywołanie ESP_LOG nadal może blokować np. za logiem innego zadania.
To ograniczenie ciągłego pakietu, nie twarda gwarancja czasu UART. Wi-Fi,
preroll/refill, dekoder, FFT i callback I2S pozostają bez zmian. Ostrzeżenia
Underrun i błędy są poza limitem jednej okresowej linii.

## Weryfikacja i odbiór

Po zmianie ten sam model1ms/20ms przechodzi z underflows0/hard0/starts1,
pełnym raportem i najwyżej jedną okresową linią w iteracji. Sprawdza też
STOP i zwolnienie właścicieli po ACK. Istniejący test Delivery sprawdza
nową nazwę i pomiar symulowanego kroku123ms. Testy zmiany stacji, generacji,
retry i błędów zachowane. Pełne wyniki host/native build w STATUS.
ASan/UBSan regresji UART PASS; LeakSanitizer wyłączony w środowisku ptrace,
bez deklaracji testu wycieków przez LSan.

Odbiór: powtórzyć LAN FLAC24 i LAN FLAC24 live headerless po3min oraz
publiczny FLAC, zachować wersję, low/hard/starts, Delivery z log_step_max_us,
PCM i CPU. Zwłaszcza sprawdzić low przy pełnym ring i wait_empty0.
Poprawa tej sytuacji nie zalicza automatycznie problemów z dopływem
publicznego streamu lub CRC. Odsłuch gdy dostępny; agent nie flashuje.
