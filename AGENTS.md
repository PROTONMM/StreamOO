# AGENTS.md — zasady dla agentów programistycznych

1. Przeczytaj `CODEX_MASTER_PROMPT.md`, `docs/00_INDEX.md` i odpowiedni numer etapu przed zmianą kodu.
2. Nie używaj Arduino Core ani nie kopiuj API WWW yoRadio. Kod yoRadio jest materiałem **referencyjnym** do zachowania, nie wzorcem implementacji.
3. Nie kopiuj kodu stron trzecich bez sprawdzenia licencji. W szczególności nie importuj kodu GPL do własnego firmware bez decyzji licencyjnej.
4. Architektura modularna: sterowniki, audio, DSP, UI, input, sieć, stacje, NVS, REST niezależne przez jawne interfejsy.
5. Wprowadzaj małe kroki. Po każdym etapie: build, testy, aktualizacja `docs/STATUS.md`, log decyzji ADR.
6. Przed rozpoczęciem nowego komponentu sprawdź API, dostępność, licencję, ograniczenia i wersję ESP-IDF zależności.
7. Nie wnioskuj pinów z dawnych definicji Arduino: źródłem jest `references/YR.kicad_sch` i `docs/02_HARDWARE.md`.
8. Krytyczna reguła: brak alokacji, operacji flash, obliczeń FFT ani blokującego I/O w ścieżce wydawania próbek I²S.
9. Nie przechowuj haseł w repo. Hasło AP ma być jednakowe dla urządzeń, lecz podawane w prywatnym procesie budowania; nie logować haseł.
10. Jeśli brak danych: napisz test/eksperyment, oznacz OPEN, nie wymyślaj niezweryfikowanego zachowania.
11. Żaden opis lub artefakt nowego projektu nie może używać starej nazwy konkretnej modyfikacji; nowy system to StreamOO, projektem źródłowym jest yoRadio.
12. Nie dodawaj do wiadomości commitów trailerów `AI-Model` ani `AI-Tokens`.
    Metryki zapisuj wyłącznie na podstawie udostępnionych, zweryfikowanych danych,
    zgodnie z `docs/COMMIT_METRICS.md`; nie wymyślaj liczników ani modeli.
