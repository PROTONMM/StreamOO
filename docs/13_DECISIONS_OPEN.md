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
| D13 | OTWARTE | Finalny format importu stacji yoRadio: potrzebny rzeczywisty plik/fixtures i test round-trip. |
| D14 | OTWARTE | Font glcdfont.c i 18 liter PL zaimportowane na wyraźne polecenie użytkownika (ADR 0006). OPEN / DO SPRAWDZENIA: autor i licencja polskich modyfikacji; użytkownik szuka źródła. |
| D15 | OTWARTE | ESP-IDF 6.1 USTALONE (v6.1 zweryfikowane, ADR 0002); nadal OTWARTE: component registry, licencje i kompatybilność kodeków. |
| D16 | OTWARTE | Dokładne zachowanie overflow/underrun, rozmiary partycji, wybór enkodera half/full quadrature po pomiarze. |
| D17 | OTWARTE | Model SSD1322 i R5/R8=4SPI potwierdzone przez użytkownika (ADR 0003); nadal OTWARTE: odbiór zasilania/resetu/obrazu OLED, modułu audio i kierunku enkodera na PCB. |
| D18 | OTWARTE | Hasło AP: konkretna wartość w prywatnej konfiguracji; uwierzytelnianie administracyjne WWW. |
| D19 | OTWARTE | Czy lista stacji ma zawijać się (wrap-around), parametr timeout menu. |
| D20 | OTWARTE | Typ formatu daty/godziny i strefy, ustawienie godziny przy niedostępnym SNTP. |
