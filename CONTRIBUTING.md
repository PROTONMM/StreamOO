# Contributing / Współtworzenie

Start with [README](README_EN.md), [AGENTS.md](AGENTS.md),
[architecture](docs/en/03_PLATFORM.md) and the relevant stage in the
[documentation index](docs/00_INDEX.md). Polish documentation starts in
[README.md](README.md). Small bug reports and focused patches are welcome.

1. Fork/clone `main`, create a focused branch, and describe a reproducible
   problem using synthetic data. Follow [build instructions](docs/en/BUILD_GUIDE.md)
   with ESP-IDF 6.1 and the checked-in dependency lock. `idf.py reconfigure`
   fetches managed components also needed by some host tests.
2. Install host tools: C/C++ compiler, Python 3.9+, FFmpeg/FFprobe and Node.js.
   For the pinned optional packages use a separate Python 3.12+ virtual environment,
   then run `python3 -m pip install -r tests/requirements-host.txt`.
   Run `python3 -m unittest discover -s tests -v`, then
   `python3 tools/check_host_safety.py` (Linux GCC, ASan/UBSan).
   Report every skip; host stubs do not execute ESP-only codec binaries.
3. Build firmware with `idf.py build`, validate the actual configuration with
   `python3 tools/check_config.py sdkconfig --platform`, and run `git diff --check`.
   A secret-free build is supported but disables networking. Supply your AP
   password privately only when preparing your own device; never upload that
   configuration, generated header, or credential-bearing firmware.
4. Preserve GPIO/partition compatibility, public interfaces and the audio
   real-time rules. No Arduino Core, no unreviewed third-party code or new
   dependencies; check licenses and SDK compatibility first. Add regression
   coverage for confirmed bugs. Optional fuzzing: [tests/fuzz](tests/fuzz/README.md).
5. Update relevant PL/EN guides, `docs/STATUS.md` and an ADR for material
   decisions. Explain what changed, why, commands/results and hardware tests
   still pending. Keep unrelated formatting out of patches. Do not claim
   hardware acceptance from a host test or fabricate commit metrics.

GitHub Actions builds the firmware using a disposable random AP key, uploads
no firmware, and runs host safety checks. Hardware
validation stays separate; attach sanitized logs when available. Contributions
to original code use the project's [MIT terms and exceptions](LICENSING.md);
preserve upstream notices. For vulnerabilities use [SECURITY.md](SECURITY.md).

PL: zgłoś odtwarzalny problem lub małą poprawkę na osobnej gałęzi, uruchom
powyższe kontrole i podaj rzeczywiste wyniki oraz pominięcia. Zachowaj warunki
licencji, dokumentację PL/EN i zasady z AGENTS.md. Nie dołączaj danych prywatnych.
