# ADR0114 — angielskie instrukcje i dokumentacja końcowa

Data: 2026-11-14. Status: wdrożone na polecenie użytkownika.

## Zakres ustalony z użytkownikiem

Użytkownik poprosił o angielską dokumentację, link na początku README.md
do README_EN.md i odsyłacz do katalogu angielskich dokumentów. Po pytaniu
o zakres wykluczył pełne tłumaczenie archiwum: „Nie, tylko instrukcję
i dokumentację końcową”. Historia etapów, raporty i pełny zbiór ADR
pozostają po polsku. Nie przedstawiamy ich jako przetłumaczonych.

## Realizacja

- README.md zaczyna się od linków do README_EN.md i angielskiego indeksu.
- README_EN.md zachowuje istniejące logo, prowadzi do docs/en/ i wersji PL.
- docs/en/ zawiera 22 dokumenty: indeks, instrukcje obsługi i budowania,
  14 rozdziałów aktualnej dokumentacji technicznej, procedury walidacji
  formatów, licencje, zasady rozwoju, stan bieżący i punkt wejścia do pracy.
- Instrukcje PL i indeks mają odsyłacze do odpowiedników EN.
- Nazwy rzeczywistych polskich przycisków pozostają przy angielskich
  objaśnieniach. Nie zmieniono języka interfejsu firmware.
- Dokumentacja techniczna opisuje obecny kod, zamiast tłumaczyć dawne
  propozycje jako aktualne funkcje. Uwzględnia m.in. rzeczywiste stany
  playera, wszystkie trasy API, dopisujący import, stabilne ID, dwa widma
  maksimum, GPIO6 enkodera i domyślne autoplay.

Wspólne pliki licencji, źródła, symbole Kconfig i API nie są duplikowane.
Linki do polskich raportów są oznaczone jako historia/dowody. Dokumentacja
końcowa nie oznacza zamknięcia wszystkich otwartych testów sprzętowych.

## Weryfikacja

- Natywny build ESP-IDF 6.1 / esp32s3: PASS.
- Walidator GPIO i profilu N16R8: PASS.
- Pełny zestaw testów hostowych: 503 PASS, bez SKIP.
- Kontrola 23 plików EN (README oraz 22 dokumenty): lokalne odsyłacze
  i kotwice, składnia 29 bloków shell i 5 przykładów JSON — PASS.
- Symbole Kconfig i 15 par metoda/trasa API porównane z implementacją — PASS.
- git diff --check: PASS.

Zmiana wyłącznie dokumentacji. Bez zmian implementacji, zależności,
sdkconfig, prywatnych danych i zasobów graficznych. Bez flash urządzenia,
nowego odsłuchu i ponawiania testów przeglądarkowych. Zachowano wcześniejsze
lokalne zmiany licencyjne i dokumentacyjne; bez commita ani push.
