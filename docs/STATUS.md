# Status implementacji

| Etap | Stan | Dowód |
|---|---|---|
| 01 zakres | wymagania zebrane | dokumentacja |
| 02 schemat i profil | walidator i menuconfig zaimplementowane; HARDWARE_TEST_PENDING | ADR 0001; 5 testów hostowych, 2332 konfiguracje; build PASS |
| 03 szkielet | utworzony — BUILD NIEPOTWIERDZONY | brak toolchain/urządzenia w pakiecie |
| 04 UI/SSD1322 | NIEZAIMPLEMENTOWANE | — |
| 05 encoder | NIEZAIMPLEMENTOWANE | — |
| 06 audio | NIEZAIMPLEMENTOWANE | — |
| 07 DSP | NIEZAIMPLEMENTOWANE | — |
| 08 stacje | NIEZAIMPLEMENTOWANE | — |
| 09 sieć | NIEZAIMPLEMENTOWANE | — |
| 10 WWW | NIEZAIMPLEMENTOWANE | — |
| 11 storage/OTA | NIEZAIMPLEMENTOWANE | — |
| 12 testy końcowe | NIEURUCHOMIONE | — |

Aktualizuj tabelę dopiero po wykonaniu i zapisaniu wyniku testów.

## Etap 02 — profil i walidacja GPIO (2026-10-24)

- Komponenty: `board_config`, Kconfig aplikacji, walidator hostowy/CMake.
- Funkcje: pełna mapa wraz z RESET, kolizje, nieistniejące GPIO, pamięć,
  boot straps, USB i konsola UART; host SPI2/SPI3 w menuconfig.
- Testy: `python3 -m unittest discover -s tests -v` — PASS (5 testów,
  2332 porównania konfiguracji C/Python); `python3 tools/check_config.py
  sdkconfig` — PASS; `git diff --check` — PASS.
- `idf.py reconfigure build` — PASS, ESP-IDF v6.1, esp32s3;
  obraz 181344 B (`0x2c460`).
- Sprzęt: HARDWARE_TEST_PENDING — zgodność sygnałów na PCB, zasilanie/reset
  SSD1322 i podciąganie enkodera. Nie wykonano flash/erase/eFuse.
- Ograniczenie: brak automatycznego wykrywania zewnętrznego JTAG;
  konserwatywne ograniczenie pinów strapujących — ADR 0001.
- Commit etapu: `git log -1 --format=%H --grep='^stage-02:'`.
  Pełny hash własnego commita nie może być zapisany wewnątrz tego commita;
  powyższe polecenie wskazuje go bez cyklicznego odwołania.
- Następny krok: etap 03 — przypięcie platformy, metryki i zadanie diagnostyczne.

Środowisko: użyto aktywatora EIM
`source /home/mariuszk/.espressif/tools/activate_idf_v6.1.sh` i
`export PATH=/usr/bin:$PATH` (CMake systemowy zamiast Snapa).
`pytest` nie jest zainstalowany; próba instalacji do `/tmp` nie powiodła się
z powodu braku DNS. Testy wykonano przez standardowy `unittest`.
