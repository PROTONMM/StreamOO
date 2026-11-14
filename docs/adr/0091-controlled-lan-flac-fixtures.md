# ADR0091 — po teście modem sleep: kontrolowane próbki FLAC w LAN

Data: 2026-11-01. Status: narzędzie i krótkie próby S3 wykonane; dłuższy odbiór OPEN.

Aktualizacja: log2cc3711b/b2431e5 obejmuje3 stacje LAN, bez Recover/Reject.
LAN16 i zwykły24bit low0, headerless jeden low przy pełnym źródle po logach.
Próby~55/78/165s, bez nowego potwierdzenia odsłuchu. Analiza i dalsza
poprawka diagnostyki w ADR0092; nie uznawać testu za zaliczenie całego radia.

## Nowy log

Załącznik45424bc0-5908-4d36-b030-a3e9d12d8a29, SHA256:
`430d969785fc184715a6575cc74fde8d276d8356aac207048f49c00f4c29121b`.
Firmware b2431e5, ELF f76a8ebd, dr_flac-0.13.4, TEST FLAC native lossless,
do183,558s. Log potwierdza `power save=NONE` i `wifi:pm start, type: 0`.
Zmiana ADR0090 została uruchomiona, lecz niedobory pozostały. Brak nowej
deklaracji odsłuchu; poprzednio użytkownik nie miał możliwości słuchania.

- 50 raportów Underrun, starts51, hard0, invalid0, max_fill90us. 23 raporty
  z compressed=1; 19 z RX age>100ms. Pierwszy low11,168s, ostatni162,048s.
  Nie porównywać wprost50 z poprzednimi97: różna długość testu i utwory.
- read_max1140965us, push_wait_max2199426us, log_max95917us. Przerwy
  odczytu przekraczają nominalny preroll500ms, ale maksima obejmują całą
  sesję i nie dowodzą przyczyny każdego low. Długie push wait przy pełnym
  ring to backpressure. Logowanie nadal zajmuje sporą część zapasu PCM.
- 35 interwałów body390..1440kbps; przepływ zależy również od materiału
  VBR i backpressure. Nie uznawać tego za zmierzony limit karty Wi-Fi.
- CPU17 interwałów: średnio40,61/42,12%, maksima47,1/47,0%. Końcowe
  FFT avg2764us/max7771us, gaps1/drop2. Brak stałego nasycenia CPU.
- Jeden Recover117,018s, skipped11848, validated19524, prefix_crc16af71.
  Brak Reject i retry po udanym gen2. Jedna początkowa porażka TLS jest
  osobnym problemem. Źródło uszkodzenia CRC pozostaje nieustalone.

## Decyzja

Nie zmieniać kolejnego parametru na podstawie innego utworu live.
Przygotować znane wejście HTTP w LAN i pozostawić firmware b2431e5.
Brak nowej wersji firmware, zmian buforów/TCP, dekodera lub logowania.

Nowe tools/generate_flac_network_fixture.py generuje własne matematyczne
sygnały przez hostowy FFmpeg, trzy pliki300s i playlist.csv w katalogu
podanym przez użytkownika. Standardowa biblioteka Pythona, bez nowych
pakietów ani kopiowania bibliotek do firmware. Istniejący FFmpeg/ffprobe
zweryfikowane przez lokalne CLI, w tym opcję muxera FLAC write_header.

| Stacja | Wejście | Co pozwala porównać |
|---|---|---|
| TEST LAN FLAC16 tones | dwa sinusy, stereo44,1kHz/16bit | lekki przypadek podstawowy |
| TEST LAN FLAC24 noise | dwa niezależne szumy z ustalonym seed, stereo44,1kHz/24bit | znacznie większy przepływ body i ramek |
| TEST LAN FLAC24 live headerless | identyczne ramki z poprzedniego pliku, stream-copy bez metadanych fLaC | dodatkowo bootstrap STREAMINFO dla live |

Ostatnia para daje identyczny PCM, bez ponownego kodowania. Poziom źródła
szumu ograniczony do amplitudy0,08. Manifest zapisuje parametry, rozmiary,
SHA256 i faktyczny średni bitrate całego pliku. Bitrate nie jest dopisywany
do icy-br ani UI. Serwer standardowy Python http.server, bez TLS i ICY.
Plik kończy się po300s audio; EOF/retry przy końcu pliku jest oczekiwany.
Oceniać pierwsze3minuty każdej stacji, zanim dojdzie do zakończenia źródła.

LAN nadal obejmuje Wi-Fi, socket HTTP, compressed ring, dekoder, PCM,
metering i OLED. Usuwa WAN, TLS, ICY oraz zmienny materiał publicznej stacji.
Pozytywny wynik dotyczy tych próbek; nie wyklucza wszystkich błędów firmware
ani nie rozstrzyga samodzielnie między WAN/TLS/ICY publicznego strumienia.

## Artefakty i weryfikacja

Gotowe lokalne pliki w ignorowanym build/flac-lan-20261101. Adres komputera
odczytano z systemu, w tej samej podsieci co radio. Adresy playlisty i media
nie trafiają do repo; prywatny główny playlist.csv pozostał nietknięty.

| Plik | Bajty | Średnia pliku kbps |
|---|---:|---:|
| lan16.flac | 7981119 | 212,83 |
| lan24.flac | 68957620 | 1838,87 |
| lan24-live.flac | 68949332 | 1838,65 |

Test hostowy3 przypadki: walidacja adresów/czasu przed zapisem, brak
nadpisywania istniejących danych, prawdziwy FFmpeg/ffprobe, powtarzalność
generacji i SHA, zgodny CSV oraz dokładny PCM32 z obu wariantów24bit.
Pełne300s: FFmpeg PCM32 SHA256 dla obu wariantów:
`041a6070a55559fcb620d625a9a4caf7aed206497afc3022a23b6d624bba15c9`.

Dodatkowo prawdziwy hostowy adapter StreamOO/dr, porcje4096B, dwa cykle
z resetem na każdym pełnym pliku24bit:211680000B PCM32 identyczne z FFmpeg,
SHA256 powtórzonego PCM:
`c6abf417dfe4c94239cb07419c042503e838955864ce3a8b1efccb3f3d083db9`.
To test kodu adaptera na hoście, nie czasu CPU/odczytu na S3.
Tymczasowy serwer loopback: pełny HTTP GET każdego z3 plików, HTTP200,
Content-Length, MIME audio/flac oraz SHA256 PASS. Serwer po próbie zamknięty.
Pełne wyniki testów w STATUS; firmware pozostaje bez zmian.

## Odbiór i kolejne rozstrzygnięcie

Procedura: FLAC_LAN_TEST.md. Uruchomić serwer, dopisać playlistę przez WWW,
wybrać po kolei3 stacje i zachować logi. Brak odsłuchu nie blokuje pomiaru,
ale nadal nie jest odbiorem brzmienia.

- Problemy już na16bit: zbadać wspólny transport/player/logowanie na S3.
- Tylko ciężkie24bit: porównać realny dopływ z wymaganym bitrate, ring,
  czasy codec/log i TCP; nie uznawać automatycznie CPU za zbyt słabe.
- Zwykłe24bit OK, headerless źle: zawęzić do bootstrap/parsera live.
- Wszystkie LAN OK, publiczny live źle: osobno dodawać TLS/ICY lub odtwarzać
  dokładny capture; dopiero te kroki rozdzielą źródło problemu.
- Recover na znanej próbce: zachować dokładne wejście z S3; poprawne bajty
  fixture i hostowego adaptera pozwalają odróżniać zmianę danych od parsera.
