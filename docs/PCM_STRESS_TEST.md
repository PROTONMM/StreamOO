# PCM — profil testu niedoboru i przeciążenia tap

Przygotowany 2026-10-25 po stabilnym przebiegu06c. Nie zmienia algorytmu
PCM/gain. Łączy dwa istniejące eksperymenty w jednym firmware, żeby odebrać
powrót po niedoborze i niezależność analizy pod obciążeniem.

Profil eksperymentalny wymaga:

```text
CONFIG_STREAMOO_PCM_STALL_TEST=y
CONFIG_STREAMOO_PCM_TAP_DELAY_MS=100
```

Normalne wartości domyślne w Kconfig pozostają n/0. Fragment
`config/pcm_stress.defaults` pozwala odtworzyć profil przy tworzeniu NOWEGO
sdkconfig, podając go po sdkconfig.defaults w SDKCONFIG_DEFAULTS. Fragment
nie zastępuje już zapisanych wartości istniejącego sdkconfig. Dla istniejącej
konfiguracji zmień te dwie opcje w menuconfig → StreamOO Configuration →
Hardware: PCM5102A (I2S), potem build. Nie kasuj konfiguracji ani NVS.

## Próba na płytce

1. Wgraj bieżący firmware jak dotychczas. W monitorze oczekuj:
   `PCM test profile: producer_stall=ON tap_delay_ms=100`.
2. Start w STOP, volume z NVS. Ustaw VOL80–100 i włącz PLAY długim kliknięciem
   w trybieA. Źródło jest ciche, jak w poprzednich próbach.
3. Po około3s od pierwszego PLAY producent zatrzyma się na400ms. Usłyszysz
   JEDNĄ celową przerwę z rampami; możliwe chwilowe BUF. Dźwięk ma sam wrócić.
4. Przez kolejne10s pozostaw PLAY. Samo opóźnienie tap działa przez cały czas,
   ale nie powinno powodować kolejnych przerw/charczenia.
5. Podeślij wiersze TEST producer stall/resumed oraz późniejsze `buffers...`
   i `PCM queued...`, a także wynik odsłuchu.

Oczekiwane po powrocie (liczniki od resetu): ACTIVE mode1, low1, hard0,
starts2; później low/starts nie rosną bez kolejnego resetu. invalid0,
max_fill wyraźnie poniżej5000us. tap_drop i tap_gaps rosną, bo odbiorca
analizy celowo nie nadąża; ich dokładne wartości nie muszą być identyczne
ze względu na oczekujące bloki i pierwszą odebraną sekwencję. tap_peak16
po ustabilizowaniu powinno wrócić do1036, również przy zmianach volume.

Przed pierwszym PLAY rosnące tap_drop przy low0/hard0 są oczekiwane:
źródło i odgałęzienie pracują przed gain także w STOP. Nie jest to analiza
rzeczywistego strumienia ani test prawdziwych widżetów.

## Po teście

Przywróć PCM_STALL_TEST=n i PCM_TAP_DELAY_MS=0 w menuconfig i przebuduj
przed zwykłym użytkowaniem lub następnym etapem. W nowej wersji komunikat
startowy pokazuje też normalne `producer_stall=OFF tap_delay_ms=0`.
NVS volume/widżetu pozostaje bez zmian. Domyślnych flag stresu nie zapisano
w sdkconfig.defaults; bieżący sdkconfig jest lokalny i ignorowany przez Git.

Build i testy hostowe nie zastępują tej próby. Bez odsłuchu/logu nie
potwierdzamy rebuffer ani odporności na przeciążenie analizy. Test nie
mierzy pełnego jitter, latencji IRQ lub wykrywania wszystkich DMA replay.

## Wynik przekazany 2026-10-25

Stall trwał400ms; po powrocie low1/hard0/starts2 pozostały stałe do70.882s,
max_fill113us i invalid0. Tap celowo tracił bloki, peak wrócił do1036.
Odbiór licznikowy pozytywny. Użytkownik następnie potwierdził odsłuch:
jedna łagodna przerwa, potem czysty ton („Tak, grał czysto”).
Po próbie lokalny sdkconfig przywrócono do n/0 i przebudowano normalny profil.
