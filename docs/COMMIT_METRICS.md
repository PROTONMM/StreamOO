# Model i tokeny dla commitów

Od polecenia użytkownika z 2026-10-24 każdy nowy commit agenta zawiera
trailery `AI-Model` i `AI-Tokens` w wiadomości Git. Dotyczy to także commitów
dokumentacji. Nie przepisujemy historii ani nie odtwarzamy fikcyjnych liczników.

Ta sesja udostępnia nazwę rodziny GPT-6 / Codex, ale nie dokładny runtime
model ID i nie licznik zużycia tokenów. Dlatego obecny zapis to:

```text
AI-Model: GPT-6 / Codex (runtime variant unavailable)
AI-Tokens: unavailable (session usage counters not exposed)
```

`unavailable` oznacza brak danych, nie zero. Nie zastępuj tego liczbą tokenów
z długości diffu, liczby słów ani estymacją kosztu. Jeżeli przyszła sesja
udostępni wiarygodną metrykę, zapisz liczbę i źródło pomiaru. Należy rozróżnić
input/output/cached/reasoning, jeśli dostawca je ujawnia, oraz zakres licznika
(np. różnica od poprzedniego commita). Sam domyślny model z lokalnego configu
nie dowodzi modelu wybranego dla konkretnej odpowiedzi.

Odczyt danych powiązanych bezpośrednio z hashem commita:

```sh
git log --format='%h %s%n%(trailers:key=AI-Model)%(trailers:key=AI-Tokens)'
```

Licznika dla wcześniejszych commitów nie dostarczono. Nie oznaczamy ich jako
pomierzonych. Trailery zapisują pochodzenie danych w samym commicie, bez
cyklicznego wpisywania jego hasha do własnej zawartości.
