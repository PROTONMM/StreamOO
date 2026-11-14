# ADR0111 — regulacja WWW podczas przesuwania suwaka

Data:2026-11-14. Status:wdrożone; odsłuch na fizycznym radiu OPEN.

Użytkownik wymaga reakcji głośności w trakcie przesuwania. Dotychczas input
zmieniał tylko podpis, a POST wysyłano dopiero na change (puszczenie).

Input teraz wysyła pierwszą zmianę bez czekania na puszczenie. Następne
początki żądań dzieli minimum100ms (maksymalnie10/s), odmierzane monotonicznym
performance.now(). Jednocześnie trwa tylko jedno żądanie. Nowe wartości
zastępują oczekującą, a końcowa wartość po change nie ginie, również gdy
odpowiedź HTTP jest wolna. Przyjęta już wartość nie jest wysyłana ponownie
na change. Blur/pointercancel kończą edycję. Bez nowego endpointu.

Suwak nie jest blokowany podczas własnego żądania. PLAY/STOP i wybór stacji
nie konkurują z oczekującą regulacją; pozostałe blokady AP/offline pozostają.
Polling nie nadpisuje przeciąganego suwaka ani jego lokalnej wartości.
configGeneration nadal odrzuca status rozpoczęty przed wysłaną komendą.
Błąd HTTP lub utrata dostępności usuwa oczekujące wartości; po powrocie
połączenia stara regulacja nie jest automatycznie odtwarzana. Rozpoczęte
żądanie kończy się zgodnie ze zwykłym timeoutem API, bez równoległych prób.

Wykorzystano istniejące polecenie volume, rampę i debounce NVS. Bez zmian
backendu, I2S, DSP ani częstotliwości zapisu ustawień. Limit100ms jest
początkowym parametrem do oceny na rzeczywistym radiu.

Walidacja:
- 27 testów portalu PASS. Cztery nowe przypadki z kontrolowanym zegarem
  i opóźnionymi odpowiedziami: natychmiastowy input, limit100ms, jedna
  operacja HTTP, zachowanie końcowej wartości (także mute0), wolne HTTP,
  brak duplikatu change, blur/cancel, błąd i odrzucenie kolejki offline.
- Lokalny Chromium390px: status syntetycznego serwera potwierdza nową
  głośność po input, PRZED change, przy aktywnym suwaku. Dotychczasowe
  przepływy AP/LAN, nawigacja, edytor/import/CAS i ustawienia PASS.
- Node syntax i git diff --check PASS. Native ESP-IDF6.1/esp32s3 build
  PASS:0x177400B,90% partycji app wolne.

Bez flash agenta. Testy z syntetycznym HTTP nie zastępują odsłuchu oraz
próby ciągłego przesuwania na fizycznym Androidzie/radiu.
