# 13 — Rejestr decyzji i otwartych kwestii

| ID | Status | Treść |
|---|---|---|
| D01 | USTALONE | Projekt StreamOO, bazowe zachowanie analizowane z yoRadio, nowa implementacja ESP-IDF. |
| D02 | USTALONE | ESP32-S3 N16R8, SSD1322 256×64, PCM5102A, jeden enkoder. |
| D03 | USTALONE | Domyślne GPIO zgodne z przesłanym schematem. |
| D04 | USTALONE | Konfiguracja sprzętu przez menuconfig, wariant A/B enkodera. |
| D05 | USTALONE | Renderer własny, Krypton + mała bitmapowa polska czcionka, układ 16/8/8/32. |
| D06 | USTALONE | FFT/bargraph/analog zmieniane przytrzymaniem i obrotem. |
| D07 | USTALONE | Głośność 0..100, wykładnicza, NVS 5 s. |
| D08 | USTALONE | Nowy REST WWW, kompatybilny format importu yoRadio. |
| D09 | USTALONE | Jedna lista stacji startowo, unikalne SSID mDNS z MAC, wspólne hasło AP, AP off po STA. |
| D10 | PROPONOWANE | Bufor 256 KiB, start 64 KiB, PCM 32–64 KiB; progi docelowo adaptacyjne. |
| D11 | PROPONOWANE | FFT 1024, opcja 2048, 24 pasma, ~25 FPS, 50% overlap. |
| D12 | OTWARTE | Czy zawsze autoplay ostatnia stacja po power-on, czy respektować zapisany STOP. |
| D13 | OTWARTE | Format prywatnego playlist.csv zweryfikowany (3 pola TAB, UTF-8); normalizacja U+200B/duplikaty, firmware importer i round-trip OPEN. ADR0030. |
| D14 | OTWARTE | Font glcdfont.c i 18 liter PL zaimportowane na wyraźne polecenie użytkownika (ADR 0006). OPEN / DO SPRAWDZENIA: autor i licencja polskich modyfikacji; użytkownik szuka źródła. |
| D15 | OTWARTE | ESP-IDF 6.1 USTALONE (v6.1 zweryfikowane, ADR 0002); nadal OTWARTE: component registry, licencje i kompatybilność kodeków. |
| D16 | OTWARTE | Dokładne zachowanie overflow/underrun, rozmiary partycji, wybór enkodera half/full quadrature po pomiarze. |
| D17 | OTWARTE | Model SSD1322 i R5/R8=4SPI potwierdzone przez użytkownika (ADR 0003); nadal OTWARTE: odbiór zasilania/resetu/obrazu OLED, modułu audio i kierunku enkodera na PCB. |
| D18 | OTWARTE | Hasło AP stałe/wbudowane z prywatnego buildu i wyświetlane na OLED w AP (potwierdzenie użytkownika2026-10-31); lokalna konfiguracja WWW AP/LAN bez dodatkowego logowania na jawne polecenie użytkownika (ADR0032); przyszła aplikacja/OTA osobno. |
| D19 | OTWARTE | Czy lista stacji ma zawijać się (wrap-around), parametr timeout menu. |
| D20 | OTWARTE | Format czasu/strefa i polityka bez SNTP nadal otwarte. W HOME 04i do testu użyto YYYY-MM-DD i HH:MM:SS z jawnego czasu cywilnego; renderer nie wybiera strefy ani nie synchronizuje czasu. |
| D21 | USTALONE | Scroll ma poruszać tekst wyłącznie od prawej do lewej; nowsza decyzja użytkownika, ADR 0009. |
| D22 | OTWARTE | Obecny font tymczasowo zaakceptowany, polskie znaki poprawne według użytkownika; docelowy wygląd czcionki wymaga dopracowania. |
| D23 | USTALONE | Tryb konfiguracji AP bez odtwarzania; dotychczasowe AAC/AP służyło testowi współpracy. ADR0028. |
| D24 | OTWARTE | MicroSD jako przyszłe lokalne źródło MP3; brak implementacji, zweryfikowanych pinów/drivera/filesystemu. |
| D25 | USTALONE | Docelowo pilot RC5; model odbiornika/GPIO, mapa i zachowanie klawiszy OPEN. ADR0029. |
| D26 | USTALONE | Docelowo aplikacja telefonu sterująca przez API StreamOO; platformy, forma aplikacji i auth/kontrakt OPEN. ADR0029. |
| D27 | USTALONE | WWW zapisuje kilka profili Wi-Fi; pierwsza dostępna według kolejności listy, z przejściem po nieudanej próbie. Do 8 profili, STA/NVS gotowe do odbioru PCB. ADR0030/0031. |
| D28 | USTALONE | WWW umożliwi upload playlist.csv; prywatny plik w katalogu głównym wykluczony z Git, bez kopiowania do build/fixture. ADR0030. |

Doprecyzowanie D03/D17: enkoder 5/6/7 i kierunek potwierdzone na sprzęcie
(ADR0015); stary schemat ma B16. W 06a model i strapping modułu DAC,
połączenia I2S oraz dźwięk/parametry analogowe nadal wymagają odbioru.

D29 — implementacja lokalnego setupu: potwierdzenie kluczem AP, preview/commit katalogu i custom storage, ADR0031. Docelowy niezależny auth administracji D18 nadal OPEN.

D30 — po STA/AP-off normalny HOME i adres IP w trzeciej linii; WWW bez sekcji dostępu i dodatkowego logowania. Jawna decyzja użytkownika zastępuje auth D29/ADR0031. ADR0032.

D20 uzupełnienie2026-10-31: użytkownik wymaga wyboru strefy przez WWW
w zakładce Konfiguracja i automatycznego DST. ADR0047 wdraża pierwsze8
stref, zapis NVS i rzeczywisty SNTP na OLED; odbiór sprzętowy OPEN.
Dotychczasowy format daty/godziny zachowany.
