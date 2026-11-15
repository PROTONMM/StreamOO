# ADR0116 — audyt repozytorium i granice parserów

Data: 2026-11-15. Status: wdrożone lokalnie; wykonanie CI na GitHub i odbiór
sprzętowy pozostają OPEN. Raport: [AUDIT_REPORT.md](../AUDIT_REPORT.md).

## Przyczyna

Niezależny przegląd wykazał akceptowanie `\u0000` przez małe endpointy JSON,
brak ograniczenia zagnieżdżenia przed rekurencją cJSON i wywołanie
`memcpy(out, NULL, 0)` w ICY. Regresja polecenia JSON oraz UBSan dla ICY
odtworzyły błędy. Stos HTTP ma 8192 B; kompilator Xtensa raportuje 64 B
dla rekurencyjnego `parse_value`, więc 255 poziomów mieści się w limicie
body 512 B, ale przekracza sam budżet stosu. Resetu płytki nie testowano.

## Decyzja

Przed cJSON dla clock/settings/player/select sprawdzamy maksymalnie cztery
poziomy i odrzucamy aktywne escape NUL w nazwach oraz wartościach. Literał
z podwójnym backslashem nie jest mylony z NUL. Wi-Fi ma już osobną bramkę
głębokości. Pusty fragment ICY kończymy przed memcpy. Interfejsy, poprawne
żądania, GPIO, partycje, kodeki i gorący tor I²S pozostają zgodne.

Dodajemy workflow z przypiętym SDK/checkoutem, dotychczasowe testy hostowe,
ASan/UBSan, analizator GCC, Cppcheck i ograniczony fuzzing. Do kompilacji
rzeczywistej sieci CI tworzy losowy jednorazowy klucz AP poza checkoutem;
nie używa klucza użytkownika i nie publikuje binarek. Fuzzery są opcjonalne,
wyłącznie hostowe. Brak nowych zależności firmware i zmian bibliotek.

Otwarte WWW jest świadomą decyzją ADR0032. Audyt nie dodaje logowania;
SECURITY.md opisuje zaufany LAN, HTTP, fizyczny dostęp i DNS rebinding.
Ustawienia GitHub oraz publikacja wymagają osobnego polecenia właściciela.

## Weryfikacja

503 testy hostowe bez SKIP; zestaw 217 testów z instrumentacją natywnych
executable ASan/UBSan; GCC i Clang na 19 przenośnych jednostkach; Cppcheck;
po 20000 wykonań pięciu fuzzerów oraz replay granic/truncation PASS.
Czysty build ESP-IDF 6.1 oraz kompilacja z jednorazowym kluczem PASS.
Szczegółowe komendy, ograniczenia oraz BLOCKED dla kampanii AFL++,
zdalnego CI i sprzętu znajdują się w raporcie. Bez flash/commita/push.
