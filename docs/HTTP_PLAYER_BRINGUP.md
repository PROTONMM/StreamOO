# Pierwszy odsłuch HTTP/ICY — 06i

Wersja gotowa do testu, nieodebrana jeszcze na ESP32-S3. Własny player
wybranej stacji, MP3 i AAC-LC/ADTS, bezpośrednie HTTP/HTTPS. Lista OLED i
konfiguracja zostały już potwierdzone; nie powtarzaj diagnostyki pinów/UART.
Szczegóły i ograniczenia: [ADR0034](adr/0034-http-icy-network-player.md).

1. Zwykłe `idf.py -p PORT flash` z dotychczasowym prywatnym kluczem AP.
   Nie kasuj NVS/flash ani nie wgrywaj obrazu FS; partycje nie zmienione.
   Playlista i profile Wi-Fi mają pozostać. W AP cisza, bez HTTP readera,
   aktywnego dekodera i I2S. Player może raportować WAIT_NETWORK/PCM=0.
2. Po STA/AP-off otwórz listę enkoderem i wybierz **bezpośredni stream MP3**.
   Sam boot nie wybiera stacji automatycznie. Zatwierdzenie stacji inicjuje
   połączenie; oczekuj tytułu „Łączenie”, potem „Buforowanie”/„Odtwarzanie”.
   Nazwa stacji w HOME, IP nadal w trzeciej linii. Brak metadanych ICY nie
   oznacza błędu — wtedy widnieje zwykły status odtwarzania.
3. Słuchaj przez co najmniej60 s. Enkoder: obrót reguluje volume, długi klik
   STOP/PLAY w trybieA (krótki w trybieB). STOP wygasza i zamyka źródło;
   PLAY otwiera tę samą stację. Sprawdź czystość, tempo, balans L/R,
   brak trzasków przy STOP i regulacji. Widzisz „Brak danych” w obszarze
   wizualizacji, ponieważ rzeczywisty DSP/tap jest osobnym etapem.
4. Zmień stację, potem wykonaj dwie szybkie zmiany. Końcowy wybór ma
   wygrać; stary tytuł nie może wrócić. Ręczny wybór/PLAY nie czeka na
   retry delay wcześniejszego błędu. Po powrocie do listy i wyborze
   aktualnej stacji może nastąpić celowy reconnect. Reorder importem ma
   zachować sesję istniejącego ID, usunięcie aktywnej pozycji zatrzymuje.
5. Po zaliczeniu MP3 sprawdź znane AAC-LC/ADTS i HTTPS. HTTPS wymaga czasu
   SNTP; pierwsza próba może się powtórzyć przed synchronizacją. Błędny
   certyfikat/niedostępny NTP nie wyłącza weryfikacji. Nie ma insecure fallback.
6. Krótki zanik danych ma wywołać fade/buforowanie i powrót, bez automatycznego
   wyboru innej stacji. Brak STA lub ponowny AP zatrzymuje I/O; po powrocie
   STA/AP-off owner ponawia tę samą pozycję. Twardy błąd/EOF ponawia po
   1/2/4/8/15s. Sam brak PCM nie wymusza restartu TCP; 5 timeoutów read
   po3s kończy sesję po około15s bez danych. DNS może trwać dłużej.

W logu player co5s:
- phase: 0STOPPED, 1WAIT_NETWORK, 2CONNECTING, 3BUFFERING, 4PLAYING, 5RETRY;
- gen, rate (źródłowe, np.44100 lub48000), compressed (do262144B), PCM (do32);
- max_fill wµs i invalid; cel: invalid=0, fill znacznie poniżej5000µs;
- low/hard/starts pozwalają ocenić rebuffer; początkowy start nie jest underrun;
- stack_hwm playera i reader_hwm — wolne bajty stosów; potrzebny bezpieczny zapas.

Prześlij log `player`/`network`/`platform` i ewentualny `decoder` po60s oraz
potwierdzenie odsłuchu, STOP/PLAY i zmiany stacji. Potrzebne heap internal
free/min/largest i PSRAM po rozpoczęciu dekodowania, a nie tylko przed nim.
Bez haseł, prywatnych URL/query ani zawartości playlisty. Przy błędzie linia
`Stream failed` podaje HTTP, decoder/vendor i format; nie przesyłaj całej listy.

HE-AAC/AAC+, FLAC/Ogg/Opus, HLS i adresy będące plikami M3U/PLS nie są jeszcze
potwierdzone/obsługiwane w tym kroku. Jeśli URL jest kontenerem/playlistą,
wybierz istniejący bezpośredni MP3/AAC-LC zamiast zmieniać oryginalny CSV.
Ostatni wybór stacji nie jest jeszcze zapisywany w NVS; po reboocie wybierz ponownie.
