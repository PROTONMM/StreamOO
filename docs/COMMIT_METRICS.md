# Model i tokeny dla commitów

Aktualizacja polecenia użytkownika z 2026-10-24: nie dodawaj do wiadomości
commitów trailerów `AI-Model` ani `AI-Tokens`. Dotyczy to także commitów
dokumentacji. Ta zasada zastępuje wcześniejszy obowiązek dopisywania trailerów.

Wcześniejszych wiadomości commitów nie przepisujemy. Wpisy w historycznych
opisach etapów odnoszą się do obowiązującej wówczas zasady.

Jeśli dostępne będą wiarygodne dane modelu i zużycia tokenów, można zapisać
je w dokumentacji z hashem commita i źródłem pomiaru. Nie dodawaj rekordów
zastępczych dla brakujących danych. Nie szacuj tokenów z diffu, liczby słów
ani kosztu; domyślny model z konfiguracji nie potwierdza modelu użytego
w konkretnej odpowiedzi.
