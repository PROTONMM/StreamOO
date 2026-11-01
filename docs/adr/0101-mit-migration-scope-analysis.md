# ADR0101: zakres analizy migracji do MIT

Data: 2026-11-01. Status: analiza; wybór migracji pozostaje po stronie użytkownika.

Użytkownik poprosił o wszystkie moduły i wymiany potrzebne do MIT.
Rozróżniono własną aplikację MIT z odrębnymi zależnościami, dodatkowe
usunięcie ograniczenia kodeków do Espressif i politykę wyłącznie MIT
dla wszystkich składników. Apache/BSD/OFL/MPL nie wymagają zmiany
licencji własnego kodu na te licencje.

Powstał MIT_MIGRATION_ANALYSIS.md:27 komponentów + main, manifesty,
fonty/CA, runtime i kandydaci MP3/AAC/FLAC/Vorbis/Opus/Ogg/TS/FFT.
MIT_MODULE_INVENTORY.csv ma114 rekordów, w tym103 archiwa z niezerową
mapowaną sekcją obrazu ELF5fa63ece. Uwzględniono również opcjonalny
micro-flac, niewłączony pcm_adapter i dane/narzędzia poza linkowaniem.
Spis jest modułowy; nie certyfikuje praw do każdego pliku bibliotek SDK.

Nowe istotne uzupełnienie wcześniejszego przeglądu: libgcc/libstdc++
z GCC Runtime Library Exception i Picolibc/libc/libm. Wyjątek nie
wymusza GPL na niezależnej aplikacji MIT spełniającej jego warunki.
Noty zachowują odrębność, podobnie jak pozostałe składniki SDK.

Rekomendacja: własny kod MIT po ustaleniu praw i rozwiązaniu małego
fontu PL; zależności z ich notami. Pełna wymiana esp_audio_codec oznacza
także parsery Ogg/TS i rozwiązanie AAC/HE-AAC. Nie ma tutaj zweryfikowanego
kompletnego zamiennika MIT AAC/HE-AAC/Opus ani stosu Wi-Fi S3. Kandydaci
z innych liberalnych licencji nie zostali nazwani bibliotekami MIT.

Sprawdzone źródła licencji upstream i lokalne, zgodność listy własnych
modułów, pokrycie archiwów i format CSV oraz diff check PASS.
Bez zmian firmware, nowych zależności, nadania LICENSE, builda i flash.
Testowany przez użytkownika kod47e44c5 pozostaje ten sam.
