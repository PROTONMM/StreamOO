# Optional host parser fuzzing

Run from the repository root after `idf.py reconfigure` fetches locked cJSON.
The harness is original project code and does not execute vendor codec binaries.
Clang with libFuzzer/ASan/UBSan is optional; normal ESP-IDF builds are unchanged.
Targets: `playlist` (M3U/PLS/URL resolution), `hls`, `icy`, `stations` (import
TSV), `wifi` (configuration JSON). Inputs are bounded to 256 KiB + 1;
individual parsers enforce their smaller limits. Seed data use `.invalid`
hosts and synthetic credentials only.

```sh
make -f tests/fuzz/Makefile -j2
mkdir -p build/fuzz/corpus/playlist
cp tests/fuzz/corpus/playlist/* build/fuzz/corpus/playlist/
build/fuzz/libfuzzer/playlist build/fuzz/corpus/playlist -max_total_time=60 -max_len=32768
```

Use the matching binary/corpus for each target. ICY is a stateful feed with
1024-byte fragments; the first input byte also selects the metadata interval.
For import size boundaries use `-max_len=262145`; longer runs are recommended.
The Wi-Fi parser also exercises malformed/truncated JSON and unknown fields.
HTTP SDK framing, TLS, demuxers and ESP-only binary decoders are outside these
targets; host integration tests cover their adapters separately.

Replay/minimize a libFuzzer crash (keep original crash files private if sourced
from real data):

```sh
build/fuzz/libfuzzer/playlist crash-HASH
build/fuzz/libfuzzer/playlist -minimize_crash=1 -exact_artifact_path=build/minimized crash-HASH
mkdir -p build/fuzz/minimal
build/fuzz/libfuzzer/playlist -merge=1 build/fuzz/minimal build/fuzz/corpus/playlist
```

For AFL++, with its compiler/runtime installed:

```sh
make -f tests/fuzz/Makefile ENGINE=afl CC=afl-clang-fast
AFL_USE_ASAN=1 afl-fuzz -m none -i tests/fuzz/corpus/playlist -o build/fuzz/afl-results -- build/fuzz/afl/playlist @@
build/fuzz/afl/playlist build/fuzz/afl-results/default/crashes/id:EXAMPLE
afl-tmin -m none -i crash-file -o build/minimized -- build/fuzz/afl/playlist @@
```

ASan/UBSan are enabled in both engines and stop on the first error. AFL requires
an environment permitting its forkserver; use its documented container/system
configuration rather than treating a blocked startup as a passing campaign.
Without an engine, `ENGINE=replay CC=clang` builds sanitized file/stdin replayers.
CI runs only a bounded smoke campaign, not a security proof. Reports must name
engine, target, duration/runs, sanitizer flags and actual findings.

PL: opcjonalne fuzzery działają wyłącznie na hoście. Korpus roboczy i awarie
trzymaj w ignorowanym `build/`; nie zmieniaj wzorcowych seedów podczas kampanii.
Brak awarii podczas krótkiego testu nie potwierdza odporności całego firmware.
