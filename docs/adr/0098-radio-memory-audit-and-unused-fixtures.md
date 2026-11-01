# ADR0098 — audyt RAM i wyłączenie nieużywanych testów audio

Data: 2026-11-01. Status: host/native PASS, heap po zmianie na S3 OPEN.

Użytkownik zlecił pierwszy krok audytu pamięci. Poprzedni FLAC/TCP23040
był stabilny w licznikach, lecz minimum internal spadło do11344B. Nie
zmniejszamy sprawdzonych kolejek/preroll/TCP ani stosów na podstawie
jednego kodeka. Pełny raport: [MEMORY_AUDIT](../MEMORY_AUDIT.md).

Normalny profil nadal miał włączone AUDIO_CONTROL_TEST i I2S_SMOKE_TEST.
Przy sieci ich zadania nie startowały, ale statyczne tap_storage7696B
i producer_block1924B pozostawały w ELF. Kontrola linkera/DWARF potwierdza
rzeczywisty koszt pamięci, nie wnioskujemy go z samej obecności źródeł.

Domyślne oba testy są teraz n, a prywatny root sdkconfig i dr uaktualniono
tylko w tym zakresie. Sieciowa bramka playera wybiera I2S_ISR_IRAM_SAFE,
defaults też jawnie y — zachowujemy bezpieczeństwo ISR niezależnie od
diagnostyki. Ręczny wybór testów w menuconfig nadal możliwy. Kod playera,
PCM, I2S fill, DSP, wyświetlacza, SNTP i polityka PSRAM bez zmian.

Po zwykłym native build .dram0.data19320→19208B, .dram0.bss40144→30464B:
**9792B mniej stałego DRAM**. Dodatkowo IRAM text mniej80B, binarka
1574288→1531776B (mniej42512B). Stare bufory/control fill i osadzony AAC
nie występują w ELF. To nie zaliczenie rzeczywistego minimum heap na S3;
kolejność alokacji, fragmentacja i ruch sieciowy wpływają na runtime.

Audyt obejmuje mapę dużych alokacji internal/PSRAM, rozmiary struktur z
DWARF, stosy skonfigurowane i minima zaobserwowane w logu oraz pomocnicze
ramki Xtensa -fstack-usage. Display ma tylko572B zapasu w badanym logu;
player/reader większy, ale nie znamy maksimum wszystkich kodeków/błędów.
Przenoszenie task-only block1924B lub workspace profilu do PSRAM i cięcie
stosów pozostają kandydatami, nie równoczesnymi zmianami tej iteracji.

474 host tests PASS, native ESP-IDF6.1 build PASS, opcje i symbole sprawdzone,
git diff --check PASS. Odbiór: standardowy flash, kilka minut FLAC i log
heap minimum/largest; odsłuch obecnie niedostępny. Potem osobny etap
presji WWW/przełączania/kodeków. Agent nie flashował.
