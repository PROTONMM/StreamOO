# Audyt pamięci StreamOO — 2026-11-01

Zakres: zwykły profil dr_flac/hardware AES/SHA/early init/TCP23040,
ESP32-S3 N16R8, ESP-IDF6.1. Zadanie użytkownika: audyt RAM, stosów i buforów
oraz znalezienie pamięci możliwej do odzyskania/przeniesienia do PSRAM.
Nie zmniejszano buforów odtwarzania, okna TCP ani stosów.

## Źródła i granice pomiaru

- Rzeczywisty log0dcbb56c/dcd5279 z ADR0095:~11min52s FLAC bez niedoborów.
  To pomiar sprzed nowej konfiguracji diagnostyki i bez odsłuchu.
- ELF/mapa zwykłego buildu przed/po wyłączeniu testowych źródeł. Rozmiary
  struktur odczytane z DWARF przez Xtensa GDB — ABI S3, nie sizeof na x86.
- Stos: konfiguracja xTaskCreate plus minima z logu; pomocniczo natywny
  kompilator Xtensa15.2.0 z -Os/-fstack-usage dla własnych funkcji.

Log sprzed zmiany: internal wolne23423..51891B podczas grania, historyczne
minimum11344B, najmniejszy raportowany largest12800B. PSRAM minimum6732956B (~6,42MiB).
Ostatni internal wraca do48339B; samo historyczne minimum nie dowodzi
wycieku, ponieważ nie rośnie po zwolnieniach. Rzeczywisty heap po zmianie,
obciążenie WWW/HLS i wszystkie kodeki wymagają dalszego pomiaru na S3.

## Znaleziona oszczędność: niewykonywany test nadal zajmował DRAM

Normalna konfiguracja miała AUDIO_CONTROL_TEST=y, AAC_FIXTURE_TEST=y
i I2S_SMOKE_TEST=y. Przy aktywnym źródle sieciowym app_main pomijał start
tych zadań, lecz linker zachowywał ich statyczne bufory i kod:

| Obiekt testowy | Bajty w poprzednim ELF |
|---|---:|
| tap_storage[4] | 7696 |
| producer_block | 1924 |
| Razem dwa główne bufory | 9620 |

To rzeczywisty koszt statyczny, nawet gdy radio grało z sieci. Nie liczymy
potencjalnych stosów/zapasów testowych jako odzyskanych: zadania nie były
uruchamiane, więc tych alokacji runtime wcześniej nie było.

Domyślnie AUDIO_CONTROL_TEST oraz I2S_SMOKE_TEST są teraz n. Wybór źródła
AAC/MP3/sine i CODEC_FIXTURE_TEST stają się nieaktywne przez zależności.
Testy pozostają dostępne przez menuconfig; nie usuwano ich implementacji.
NETWORK_AP_TEST (historyczna bramka sieci/playera) jawnie wybiera
I2S_ISR_IRAM_SAFE; defaults również zachowują y. Bez tego wcześniejsze
select z wyłączanych testów mogłoby zniknąć w świeżej konfiguracji.

### Pomiar sekcji przed/po

| Sekcja | Przed [B] | Po [B] | Odzyskane [B] |
|---|---:|---:|---:|
| .dram0.data | 19320 | 19208 | 112 |
| .dram0.bss | 40144 | 30464 | 9680 |
| **Stały DRAM razem** | **59464** | **49672** | **9792** |
| .iram0.text | 90223 | 90143 | 80 |
| .flash.text | 1051676 | 1047468 | 4208 |
| .flash.rodata | 411580 | 373468 | 38112 |

Binarka1574288→1531776B, mniej o42512B (~41,5KiB). IRAM i DRAM pokazano
osobno, bez podwójnego liczenia .dram0.dummy aliasującego obszar IRAM.
Nie wliczano sekcji debug, paddingu ani flash w odzyskany DRAM.

Po zmianie ELF nie zawiera tap_storage, producer_block, controlled_fill
ani osadzonego AAC channels_48k_stereo_aac. Nagłówek wygenerowany przez SDK
potwierdza wyłączone testy oraz I2S_ISR_IRAM_SAFE=y. 9792B to twardy wynik
linkera, nie pomiar nowego minimum heap na płytce. Przy porównywalnym
obciążeniu oczekujemy większego zapasu; nie obiecujemy dokładnego minimum
ani największego bloku po fragmentacji/zmianie kolejności alokacji.

## Główne bufory i ich własność

Rozmiary dotyczą payloadu/struktur, bez narzutu heap, RTOS i sterowników.
Wiersze zawierające agregaty nie są sumowane z ich polami drugi raz.

| Obiekt | Rozmiar S3 | Pamięć / czas życia | Ocena |
|---|---:|---|---|
| Kolejka PCM32 × streamoo_pcm_block_t | 61568B | internal, sesja, ISR konsumuje | Zachować rozmiar i położenie |
| Roboczy block playera | 1924B | internal, sesja, tylko producent | Kandydat do PSRAM osobnym krokiem |
| Pipeline PCM | 80B | internal, ISR | Zachować |
| Tap + metering (cały kontekst) | 9880B | internal, sesja | Zawiera ring4 ×1924B oraz block1924B; ISR zapisuje tap |
| I2S DMA4 ×240 ×2 ×int32 | 7680B | internal DMA, sesja | Plus deskryptory/SDK; zachować |
| SPI OLED staging | 8192B | internal DMA, życie ekranu | Zachować działający transfer/timingi |
| Framebuffer OLED | 8192B | PSRAM, życie ekranu | Już poza internal |
| Cache poprzedniej klatki OLED | 8192B | PSRAM, życie ekranu | Osobny od staging; używany do dirty rows |
| Sesja HTTP (struct) | 33184B | PSRAM, reader do ACK zamknięcia | URL, ICY, markery; już poza internal |
| Compressed ring | 262144B | PSRAM, sesja | Zachować, pełny w dobrym teście |
| Scratch live decoder | PCM1MiB + input4KiB i stan | PSRAM, sesja | Nie powiększać/przenosić do internal |
| Spectrum stream | 28832B | wyrównany PSRAM, zadanie analizy | SIMD work już w PSRAM, poza I2S |
| SIMD sc16_twiddle | 4096B | statyczny internal, życie firmware | Możliwe dalsze badanie; nie przenoszono |
| cpu_tasks + cpu_previous | 1920B +768B | statyczny internal, profiler | Kandydat do PSRAM po sprawdzeniu użycia z RTOS |
| Katalog/import/JSON WWW | zmienny | główne duże bufory jawnie PSRAM | Zmierzyć szczyty przy aktywnym WWW |
| Wi-Fi/lwIP/TLS/kodeki | zmienny | SDK, zależnie od konfiguracji | Nie sumować payloadu okna TCP z heap bez pomiaru |

MALLOC_ALWAYSINTERNAL=16384, RESERVE_INTERNAL=32768. Rezerwa nie jest
dodatkową pamięcią do dodania do heap free. Typowe małe alokacje/pakiety
nadal preferują internal; SPIRAM_TRY_ALLOCATE_WIFI_LWIP=n. TLS główne
bufory mają EXTERNAL_MEM_ALLOC. Pozostałe alokacje SDK mogą mieć własne
wymagania i chwilowe piki; sześć MiB PSRAM nie jest zamiennikiem RAM DMA.

## Stosy

W ESP-IDF podane rozmiary i high-water mark są w bajtach. Kolumna zapasu
dotyczy wyłącznie tego logu FLAC. Profiler raportuje tylko sześć zadań
o największym użyciu CPU w danym interwale, więc brak w tabeli pomiaru
nie oznacza pełnego lub nieużywanego stosu.

| Zadanie | Przydzielone [B] | Najmniej wolne w logu [B] | Decyzja |
|---|---:|---:|---|
| player | 24576 | 16688 | Duży kandydat, ale najpierw inne kodeki/błędy |
| http_reader | 16384 | 10540 | Najpierw TLS/HLS i złożone playlisty |
| audio_levels | 4096 | 2624 | Zostawić |
| display_smoke | 4096 | 572 | Mały zapas; nie zmniejszać |
| network | 8192 | brak w top6 | Potrzebny pomiar provisioning/reconnect |
| httpd | 8192 | brak w top6 | Potrzebny import/edycja/błędne payloady |
| input_probe | 4096 | 2868 | Możliwy kandydat po szerszym pomiarze |
| streamoo_health | 3072 | 1152 (osobny log) | Zostawić |
| preferences | 3072 | brak w top6 | Potrzebny pomiar NVS |
| system_settings | 4096 | brak w top6 | Potrzebny pomiar NVS |

Suma tych przydziałów wynosi79872B, bez TCB/kolejek i zadań SDK. Nie dodawać
ponownie stosu main3584B jako stałego: app_main kończy się, a zadanie main
jest usuwane. Osobne stosy SDK (Wi-Fi, TCP/IP, sys_evt, timer, idle) oraz
stosy ISR też pozostają w budżecie. Nie przenoszono stosów do PSRAM:
wywołania flash/NVS i wymagania SDK trzeba ocenić oddzielnie.

Pomocnicze ramki -fstack-usage obecnego kodu (-Os): player task2000B,
HTTP reader2864B, metering analyze416B, display_task1888B, log_cpu560B,
health_task48B; parser HLS2240B. To **pojedyncze ramki**, nie całe drzewo
wywołań, biblioteki binarne, przerwania, rekursja ani dowód bezpiecznego
zmniejszenia stosu. Nie odejmowano high-water z FLAC od przydziału w celu
automatycznego ustawienia nowych rozmiarów.

## Wynik i dalszy odbiór

Wdrożona tylko oszczędność nieużywanej diagnostyki. 474 testy hostowe PASS,
w tym start bez diagnostycznego audio z siecią i bez niej; opt-in testy
ciągłe i finite zachowane w istniejących scenariuszach. Native ESP-IDF6.1
build PASS, porównanie ELF i wygenerowane opcje potwierdzone. Osobna świeża
generacja defaults potwierdza wyłączone testy i IRAM-safe I2S. Agent nie
flashował; nowe minimum/largest na płytce OPEN.

Wgrać zwykłym idf.py, zebrać początkowy heap oraz kilka minut logu FLAC.
Nie trzeba teraz odsłuchu. Następny osobny etap: WWW/przełączanie/HLS/TLS
i pozostałe kodeki z obserwacją minimum/largest oraz stosów. Dopiero wtedy
dobierać kolejną oszczędność. Mapa kandydatów powyżej nie oznacza, że
wszystkie przeniesienia zostały zaimplementowane lub sprawdzone.

Lokalne artefakty (ignorowane przez Git): build/memory-audit/before-sections.txt,
after-sections.txt, before-symbols.txt, after-symbols.txt, section-comparison.json.
Wybrane .su z dodatkowej kompilacji: /tmp/streamoo-stack-audit. Bazowy ELF
/tmp/streamoo-memory-before.elf (plik prywatny0600, nie do repo).
Do powtórzenia analizy użyto narzędzi istniejącego SDK: Xtensa size -A,
nm -S --size-sort, GDB p sizeof(...), kompilacja z -fstack-usage. Bez nowych
zależności i bez kopiowania implementacji stron trzecich.
