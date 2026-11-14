# Odbiór obsługi i ustawień radia — ADR0059

Zwykły `idf.py -p PORT flash`, bez erase i obrazu FS. Ctrl+F5 w WWW.
Partycje i prywatny playlist.csv bez zmian. Nowy firmware czyta dotychczasowy
katalog; zapisy edytora zachowują ID także po zmianie nazwy/URL.

1. Przy grającej stacji krótki klik otwiera listę. Zatwierdzenie tej samej
   pozycji nie przerywa audio: generation/starts i liczba połączeń bez zmian.
2. Długi klik z HOME lub listy zatrzymuje i wygasza OLED. WWW pozostaje
   dostępne. Krótki klik wybudza HOME i rozpoczyna PLAY tej samej stacji;
   pierwsze próbki po normalnym buforowaniu. WWW STOP/PLAY też gasi/wybudza.
3. Domyślnie po5s bezczynności lista gra zaznaczoną stację. Zmień przez
   LAN→Konfiguracja czas np.9s oraz powrót bez zmiany; sprawdź obie opcje.
   Obrót odnawia czas. Zaznaczona ta sama grająca pozycja pozostaje no-op.
4. Wybierz stację i odczekaj co najmniej5s. Start STOP: po power-cycle
   zapamiętany wybór, OLED wygaszony po STA, krótki PLAY. Start autoplay:
   po boocie i STA/AP-off ta sama stacja. Przy braku jej ID w katalogu
   brak automatycznego wyboru innej. AP zawsze pokazuje konfigurację.
5. WWW edytor: zmień nazwę/URL grającej stacji, zapisz. Audio bez restartu,
   ID ten sam; nowy URL przy kolejnym połączeniu. Reorder bez restartu,
   usunięcie aktualnej zatrzymuje. Eksport pozostaje3-polowym CSV.
6. Konfiguracja: ustaw np. kitchen-radio; otwórz `http://kitchen-radio.local/`.
   Audio nieprzerwane. Puste pole przywraca nazwę MAC. Sprawdź trwałość
   timeout/akcji/startu/nazwy i ID po fizycznym wyłączeniu zasilania.

Hosty obejmują te przejścia na atrapach SDK, ale nie potwierdzają fizycznego
OLED, multicastu, DMA i odporności na odłączenie zasilania. Wynik PCB OPEN.
