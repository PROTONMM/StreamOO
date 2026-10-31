# ADR0063 — startowe logo dopasowane do SSD1322

Data:2026-10-31. Status: propozycja zaimplementowana, wygląd OLED OPEN.

Użytkownik wymaga logo tylko po podłączeniu zasilania lub resecie i pyta
o wykonanie go w ChatGPT w przeglądarce. Dla256×64 wybieramy oryginalny
znak renderowany przez istniejący canvas4bpp i fontKrypton: dwa O/fale,
StreamOO, linia sygnału oraz przyciemniony podpis INTERNET RADIO. Bez
nowej biblioteki/fontu, bez generowania AI/konwersji obcej bitmapy.

`streamoo_ui_boot_draw`: dokładnie8192B, odrzuca błędny rozmiar, bez heap.
Pierwszy1024B wspólnego framebuffer to tymczasowa powierzchnia napisu;
2× upscaling w niepokrywającym się obszarze, potem czyszczenie stagingu.
Brak drugiego framebuffer i dużej tablicy na stosie4096B UI. Istniejący
Krypton/OFL nadal ma niezmienioną atrybucję; znak i linia własne.

Przed pierwszym present/enable ekran dostaje logo. Po sukcesie flaga RAM
ustawiana raz na boot, task UI śpi1500ms, pozostałe taski Wi-Fi/player nie
są blokowane. Potem zwykły HOME/STA/AP lub wygaszony STOP. PLAY po pauzie
wykonuje istniejące enable/present, nie wywołuje inicjalizacji/logo. Reconnect
sieci i zmiana stacji również nie pokazują logo. Każdy reset procesora
czyści flagę; nie zapisujemy jej w NVS.

Natywny podgląd assets/branding/startup-256x64.png i powiększenie4×
sprawdzone wizualnie. tools/preview_boot_logo.py kompiluje rzeczywisty
renderer i sprawdza guard bytes/błędny rozmiar; PNG bez zależności Pythona.
Native build PASS razem z ADR0062 (0x17a8a0/90% app), testy hostowe370 PASS.
Nie zastępuje to odbioru jasności/ostrości/timingu OLED na PCB.

Odbiór po zwykłym flash bez erase: reset i power-cycle→logo1,5s;
STOP/PLAY, reconnect, zmiana stacji i przejście AP→STA→bez powtórzenia logo.
Nie wykonano flash/erase/eFuse. Projekt graficzny jest propozycją do oceny,
możliwą do zmiany niezależnie od audio. GPIO/partycje/ustawienia bez zmian.
