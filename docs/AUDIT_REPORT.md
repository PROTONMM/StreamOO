# StreamOO repository audit

Date: 2026-11-15. Baseline: `5aec93a3dfc568e5b0e77c05eddc85ac7f112555`
(merged v0.1.0). Work branch: `audit/comprehensive-review-20261115`.
The initial tree was clean, on an existing `audit/comprehensive-review` branch;
a separate branch was created before edits. No commits, pushes, releases,
tags, repository settings, device flash or eFuses were changed.

PL: audyt obejmuje kod, bezpieczeństwo, build, testy i przygotowanie repozytorium.
Naprawiono potwierdzone błędy JSON/ICY, dodano CI i fuzzery; testy sprzętowe
oraz wykonanie workflow na GitHub pozostają otwarte. Decyzja: [ADR0116](adr/0116-repository-audit-and-parser-safety.md).

## Executive summary

StreamOO has a substantial modular implementation, explicit audio/network/UI
owners, bounded network parsers, persistent catalog snapshots and an existing
503-test host suite. Its documentation and license collection are extensive.
The first public source release **already exists**; reports of missing Issues
or release tags were incorrect at audit time.

Three concrete problems were addressed: recursive JSON parsing could exceed
the HTTP task's stack budget; JSON strings/keys containing decoded NUL passed
prefix-based validation; and empty ICY input invoked undefined behavior.
Only two firmware source files changed. No critical/high severity defect was
confirmed within this review. This is a scoped engineering audit, not proof
that arbitrary network data or third-party binary codecs are vulnerability-free.

CI and repeatable host safety/fuzz targets are now prepared locally. The
firmware builds cleanly with ESP-IDF 6.1, both without an AP key and with a
disposable key compiling the real network owner. Host tests, sanitizers,
static checks and browser checks passed. Target runtime reliability still
needs hardware; no new CPU/latency/heap benchmark is claimed.

## Findings

### AUD-01 — Medium — network input / stack safety — fixed

**Affected:** `components/portal/streamoo_portal_config.c`, `settings_post`,
`clock_post`, `player_post`, `station_select`; HTTP stack configured in
`components/portal/streamoo_portal.c::streamoo_portal_start`.

**Evidence/root cause:** size limits alone do not bound cJSON recursion.
Settings accepted up to 512 bytes before calling `cJSON_ParseWithLengthOpts`.
`'[' * 255 + '0' + ']' * 255` is 511 bytes; cJSON's default nesting limit is
1000. Recompiling the actual pinned cJSON command with `-fstack-usage` reports
64 bytes for recursive `parse_value` on Xtensa, with array parsing inlined.
255 such frames require 16,320 bytes before caller overhead; HTTP has 8192.
Schema rejection occurs after parsing. The Wi-Fi parser already bounds depth.

**Impact:** a LAN client able to send the required custom header can exhaust
the server task stack, potentially resetting the device. This is compiler and
source evidence; no target reset/exploit was executed.

**Fix:** linear, escape-aware preflight bounds nesting to four before cJSON,
shared by the four small JSON handlers. No larger stack or global cJSON
configuration change. **Validation: PASS** source/target stack analysis,
maximum nested request regression, full host suite, native sanitizer harness
and firmware compilation. **BLOCKED:** physical target reproduction/acceptance.

### AUD-02 — Low — request validation — fixed

**Affected:** same four JSON handlers; new `parse_json` preflight.

**Evidence/root cause:** raw `memchr` rejected literal NUL bytes, but cJSON
decodes `\u0000` into C strings. `{"action":"volume\u0000ignored","value":57}`
was accepted as `volume`. Adding the regression before the fix failed an
assertion expecting HTTP 400. A similar prefix could appear in field names,
time zones, station IDs and hostnames.

**Impact:** ambiguous/malformed requests accepted contrary to strict schemas.
This is not an authentication bypass: the LAN API intentionally has no login.
**Fix:** reject active NUL escapes in JSON keys and values; distinguish an
escaped literal backslash from a Unicode escape. **Validation: PASS** cases for
action, key, timezone, station ID and hostname, plus escaped quotes/backslashes,
normal requests, malformed payloads and ASan/UBSan. The synthetic settings
backend intentionally accepts arbitrary hostname text for the escaping test;
production hostname validation remains unchanged.

### AUD-03 — Low — undefined behavior — fixed

**Affected:** `components/streaming/streamoo_icy.c::streamoo_icy_feed`.

**Evidence/root cause:** argument validation permits `in == NULL` when `n == 0`.
With metadata disabled it nevertheless called `memcpy(out, in, n)`. A new
empty-input case failed UBSan before the fix: `null pointer passed as argument
2, which is declared to never be null` (old line 45).

**Impact:** undefined behavior in an accepted API call; no remote overflow or
observed audio fault is established. **Fix:** return successfully with zero
written bytes before memcpy for empty input, preserving normal behavior.
**Validation: PASS** original red reproducer, corrected ASan/UBSan live-stream
tests, arbitrary ICY fragmentation, full suite and ICY fuzz/boundary replay.

### AUD-04 — Medium — deployment security — intentional residual risk

**Affected:** `streamoo_portal_config.c::request_allowed`, configuration/control
routes, `streamoo_portal.c::streamoo_portal_start`; storage/build secret policy.

**Evidence:** LAN and AP HTTP APIs intentionally require no user login under
ADR0032. The custom request header, matching Origin when supplied, no CORS,
strict content types and CSP already exist. Any direct LAN client can supply
that header; HTTP exposes submitted Wi-Fi credentials to an on-path attacker.
The default profile does not enable secure boot, flash encryption or NVS
encryption. Host/Origin equality is not a hostname allowlist, so DNS rebinding
remains a threat-model concern, not a reproduced exploit in this audit.

**Impact:** untrusted LAN access permits playback/configuration changes;
physical access may disclose embedded AP/NVS credentials. **Action:** added
`SECURITY.md`, preserving the explicit open-local-panel requirement. Use a
trusted isolated LAN, no port forwarding, protect firmware and private files.
Authentication/TLS management or Host allowlisting needs a separate design
decision covering IP, configured mDNS, DHCP and future clients.
**Validation:** current guards and open-access behavior **PASS** host/browser
tests; DNS rebinding, on-path and physical attack exercises **BLOCKED/not run**.

### AUD-05 — Low — continuous verification — remediated locally

**Affected:** `.github/workflows/ci.yml`, `tools/check_host_safety.py`, `tests/fuzz`.
**Evidence:** no `.github` directory at baseline; GitHub workflows API returned
`total_count: 0`. Existing tests were extensive; the missing part was automation,
not test infrastructure.

**Fix:** one bounded CI job, pinned checkout SHA and official ESP-IDF 6.1 image
digest, read-only repository permission and no persisted checkout credentials.
It builds firmware, fetches locked components, runs all host tests with skips
treated as incomplete coverage, native sanitizer tests, GCC analyzer, Cppcheck
and short fuzz campaigns. A random disposable AP key outside checkout compiles
the real network path. It uploads no credential-bearing binaries. No firmware
dependency, SDK setting or global warning suppression was introduced.
**Validation: PASS** actionlint 1.7.7 and local checks below.
**BLOCKED:** GitHub execution awaits an explicitly approved push; no green
GitHub run is claimed. Container image digest/tag availability was checked via
Docker Hub's official API; the exact container job was not executed locally.

### AUD-06 — Low — contributor documentation — fixed

**Affected:** PL/EN `BUILD_GUIDE.md`, root repository guidance.
**Evidence:** both guides instructed cloning `feature/streamoo-implementation`
and described PR #1 as pending; local merge and GitHub default branch/release
show `main`/v0.1.0. `SECURITY.md`, `CONTRIBUTING.md` and `CHANGELOG.md` were absent.
**Fix:** current checkout/tag instructions, concise bilingual repository
guidance, actual release history and links to existing detailed manuals.
**Validation: PASS** links, commands/source comparison and diff checks.

### AUD-07 — Low — repository maintenance — recommendations only

**Affected:** GitHub repository settings/metadata, not firmware.
**Evidence (authenticated read-only API):** Topics empty; description only
`Internet Radio`; private vulnerability reporting disabled; Dependabot security
updates, secret scanning and push protection disabled. Issues **enabled**,
release/tag `v0.1.0` **present**, published 2026-11-14, no binary assets.
The community profile API reports no contribution/issue templates at baseline;
this does not indicate Issues are restricted to maintainers.

**Action:** local contribution/security/changelog files added. Administrative
commands below are recommendations, not executed changes. **Validation: PASS**
metadata queries; enabling features **BLOCKED pending owner approval**.

### AUD-08 — Informational — release/runtime evidence — remains open

**Affected:** audio/decoder/DSP/network/storage acceptance protocols.
**Evidence:** existing status explicitly leaves broader codec profiles, sustained
bitrate, long-running FLAC/network stability, stack margins and power-interrupted
writes open. Host vendor API stubs cannot execute Espressif binary codecs or
validate DMA timing. **Impact:** source release is usable development firmware,
not a fully qualified appliance. **Action:** retain honest development labeling;
complete hardware protocols. **Validation: BLOCKED** without device access.

## Technical review coverage and preserved mechanisms

| Area | Source evidence and conclusion |
| --- | --- |
| Build | Root/component/main CMake, manifests and lock checked. Exact SDK major/minor and S3 target gates; explicit component include/dependency lists; public headers with IDF types have required dependencies. Native clean compilation found no missing includes/dependencies or compiler warnings. Main's broad component list is maintainability overhead, not a demonstrated defect. No aggressive flags added. Size optimization and silent assertions retain checks; no fast-math/strict-aliasing workaround introduced. |
| Ownership/concurrency | Player queue serializes encoder/WWW commands; retained catalog snapshots use critical-section reference counts and generation/CAS checks. PCM SPSC publication uses aligned acquire/release atomic loads/stores. Buffered compressed input uses a bounded mutex attempt. Reader stop/done and metering stop/done acknowledge before disposal; I2S shutdown precedes ring/tap release. Host tests exercise stale generations, queue rejection, threaded PCM transfer, stop/retry/allocation failures and station replacement. No new confirmed race/deadlock was reproduced. No TSan or target scheduling proof is claimed. |
| Audio | HTTP/PSRAM ring → parser/decoder → PCM conversion/resample to 48 kHz → pre-gain tap → ramp → DMA. ISR fill performs fixed bounded processing without allocation, flash, FFT or blocking I/O. Ring space is reserved before TLS to reduce contiguous-heap failures. Decoder consumed/produced capacities and format changes are checked. Temporary underruns retain the stream. EOF/retry, partial frames, resampler phase and integer PCM extremes have existing executable tests. Linear interpolation remains intentional; no resampling quality claim. |
| Codec/parser memory | Inspected decoder adapters, FLAC header/CRC parser and dr_flac bridge: fixed caches, allocation checks, consumed/produced limits and incremental metadata skip. Default dr_flac and alternative micro-flac have host FFmpeg oracle tests. Vendor ESP-only binaries are not source-audited or fuzzed here. Third-party implementation correctness is not inferred from adapter tests. |
| Network | URL/playlist size limits, three nested playlist/master levels, five redirects per resource, HLS segment/manifest limits, timeout/cancel handling and no HTTPS downgrade already present. TLS bundle/hostname/time validation and interruptible UTC gate remain enabled. HLS sequence overflow is already checked in parser/reader; it was not reported as a new bug. SDK handles HTTP Content-Length/chunk framing; adapters are tested with transport fakes, not claimed as real TLS wire fuzzing. |
| Web | Mutation routes consistently call request guard; AP/LAN catalog/player gates already exist. Request bodies have endpoint limits; malformed/rejected requests close the session. cJSON serializes JSON; frontend uses DOM/textContent for externally sourced station data. CSP, nosniff and frame restrictions already present. No new XSS/CSRF bypass was confirmed. Direct LAN authorization and browser DNS threats are distinct; see AUD-04. |
| Storage | Explicit versioned byte serialization, profile checksums/validation and persistent ID format; bounded read sizes. LittleFS writes temporary catalog, flushes/fsyncs/closes then renames before publishing. Errors retain the prior catalog. Partition-specific first-format marker and nonblank guards prevent indiscriminate formatting. Preferences/station writes are deferred; time zone avoids a persisted no-op write. Ordinary settings_set may still commit unchanged settings; this is an optimization opportunity, not a demonstrated flash-lifetime failure. |
| DSP | Hann N−1, weighted DC removal, stereo power combination, saturated fixed-point conversion, 16-byte aligned sc16 work/twiddle table, separate droppable-tap worker. Existing NumPy and real ESP-DSP ANSI comparisons exercise scale/ordering; host aliases AES3 to ANSI and does not prove target SIMD speed or instruction timing. No extra buffer copy or optimization added. |
| Licenses | Root MIT, scope exceptions, vendor Modified MIT/Espressif-only restriction, font licenses, retained CA/source and runtime notices already present. Verified 9 retained-license hashes and 24 vendored-source hashes against checked-in provenance. No third-party source added/updated. Historical reference materials remain outside the original-code MIT grant; no legal clearance for unresolved reference rights is asserted. |

This coverage prioritizes externally controlled input, active playback owners
and their existing tests. It is not a line-by-line certification of every SDK
component, vendored source or historical Git object. Optional diagnostic CPU
profiling retains task handles/name pointers from `uxTaskGetSystemState` before
later logging/affinity reads: stress rapid reader deletion under diagnostics
to evaluate snapshot lifetime. This is an unverified lifecycle concern, not a
confirmed use-after-free; no speculative scheduler refactoring was applied.

## Changes implemented

- Firmware: escape-aware shallow JSON preflight and zero-length ICY handling.
- Regression cases in `tests/test_portal_configuration.py` and `test_live_stream.py`.
- CI workflow; pinned optional host Python requirements; reusable native safety
  runner with 19-unit GCC analysis; five optional libFuzzer/AFL++/replay targets
  and seven small original valid seeds, instructions for replay/minimization.
- Root `SECURITY.md`, `CONTRIBUTING.md`, `CHANGELOG.md`; PL/EN build fixes;
  README/index/test-guide links, ADR0116 and status/resume entries.

No pin assignments, partition layout, dependency versions, codec behavior,
audio thresholds, optimization settings or production sdkconfig were changed.

## Test results and reproducible commands

Environment: Linux x86-64, Python 3.12, GCC 13.3, Clang 20.1.8,
Cppcheck 2.13.0, ESP-IDF v6.1 / Xtensa GCC 15.2.0. Optional test packages:
NumPy 1.26.4, Pillow 10.2.0, fonttools 4.46.0. Baseline and final unit runs
both completed **503 tests, no skips**; regressions extend existing harnesses
rather than inflating test counts.

The new optional CI requirements use separately verified PyPI releases
NumPy 2.5.4, Pillow 13.0.0 and fonttools 4.68.0 (Python 3.12+), in a dedicated
host venv so SDK packages are unaffected. A further complete run in that fresh
venv completed **503 tests in 36.154 s, no skips**. The existing basic suite
can still run with Python 3.9+ and report unavailable optional oracles as skips.

| Validation | Result | Actual execution/evidence |
| --- | --- | --- |
| Baseline suite | PASS | `python3 -m unittest discover -s tests -v` — 503 in 34.833 s |
| Red JSON reproducer | FAIL, expected before fix | `python3 -m unittest discover -s tests -p test_portal_configuration.py -v` — new HTTP 400 assertion failed |
| Red ICY reproducer | FAIL, expected before fix | `STREAMOO_LIVE_SANITIZE='-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -g -O1 -no-pie' python3 -m unittest discover -s tests -p test_live_stream.py -v` — UBSan NULL memcpy |
| Final full suite | PASS | same discover command — 503 in 52.948 s, no skips |
| Fresh pinned host environment | PASS | `python3 -m venv /tmp/streamoo-audit-host-venv`; installed the three pinned optional packages; `/tmp/streamoo-audit-host-venv/bin/python -m unittest discover -s tests -v` — 503 in 36.154 s, no skips |
| Native sanitizer suite | PASS | `python3 tools/check_host_safety.py` — 217 selected tests in 85.232 s, no skips; native executable portions instrumented, ordinary Python/ctypes shared libraries are not fully instrumented |
| GCC analyzer | PASS | same script — `gcc -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -fanalyzer -O1 ... -c` for 19 listed portable project units |
| Clang analyzer | PASS | `clang --analyze -Xanalyzer -analyzer-output=text -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror` with component include paths, same 19 units; no diagnostics |
| Cppcheck | PASS | `cppcheck --enable=warning,performance,portability --std=c11 --inline-suppr --error-exitcode=1 -i components/decoder/third_party -i components/text/generated components main` — 76 files/configurations checked, no diagnostics; SDK headers/target behavior not modeled fully |
| Extra canvas/gain stress | PASS | compiled existing `canvas_sanitizer.c` and `gain_sanitizer.c` with native `cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -g -O1 -no-pie`, existing source/include lists in tests/README.md; executed both |
| libFuzzer | PASS | `make -f tests/fuzz/Makefile -j2`; each of five binaries with copied corpus, `-runs=20000 -max_len=32768 -seed=20261115`; 100,000 total executions, no crash |
| Boundary/truncation replay | PASS | each fuzzer with `-runs=0 -max_len=262146`, corpus containing all valid-seed truncations plus deterministic random lengths 0,1,3,16,128,4095,4096,4097,16384,16385,262144,262145,262146 |
| AFL++ build/seed replay | PASS | `make -f tests/fuzz/Makefile ENGINE=afl CC=afl-clang-fast -j2`; all five binaries executed on their valid seeds |
| AFL++ campaign | BLOCKED | `AFL_USE_ASAN=1 afl-fuzz -V 10 -m none -i tests/fuzz/corpus/playlist -o build/fuzz/afl-smoke -- build/fuzz/afl/playlist @@` aborted because core_pattern pipes to an external collector. No crash-detection bypass or system setting change |
| Browser behavior | PASS | `python3 tools/check_portal_browser.py` — AP/LAN/control/editor/settings flows, Chromium 390px |
| Browser layouts | PASS | `python3 tools/check_portal_layout.py` — six views 320–1280px, light/dark/system, editor |
| Clean ESP-IDF build | PASS | commands below, separate /tmp sdkconfig/build; fullclean then build — secret-free image 0x10d500 B |
| Real network owner build | PASS | reconfigure/build using disposable random external AP key — image 0x177490 B (1,537,168), 90% app free; no compiler warnings in build logs |
| GPIO/platform | PASS | `python3 tools/check_config.py /tmp/streamoo-audit-sdkconfig --platform` |
| Workflow syntax | PASS | `/tmp/streamoo-audit-actionlint/actionlint .github/workflows/ci.yml` (v1.7.7) |
| GitHub Actions execution | BLOCKED | workflow is local; push not authorized |
| Documentation/licenses/diff | PASS | local Markdown links in changed guides checked; 9/24 provenance hashes verified; `git diff --check`; changed-file secret-pattern review |
| Hardware/soak/power loss | BLOCKED | no device tests or flash performed |
| Aggressive optimization benchmark | NOT APPLICABLE | no performance optimization proposed or applied |

Firmware commands actually used (the local production configuration is retained):

```sh
source /home/mariuszk/.espressif/v6.1/esp-idf/export.sh
export PATH=/usr/bin:$PATH
export XDG_CONFIG_HOME=/tmp/streamoo-audit-no-secrets
idf.py -B /tmp/streamoo-audit-build -D SDKCONFIG=/tmp/streamoo-audit-sdkconfig -D IDF_TARGET=esp32s3 build
idf.py -B /tmp/streamoo-audit-build -D SDKCONFIG=/tmp/streamoo-audit-sdkconfig -D IDF_TARGET=esp32s3 fullclean build
# Created with tools.configure_network.save_private + secrets.token_urlsafe(24),
# without printing the disposable value:
export XDG_CONFIG_HOME=/tmp/streamoo-audit-private
idf.py -B /tmp/streamoo-audit-build -D SDKCONFIG=/tmp/streamoo-audit-sdkconfig -D IDF_TARGET=esp32s3 reconfigure build
```

The clean build reused checked-out locked managed components and installed
toolchains; this is not an offline fresh-machine dependency-download test or a
bit-identical build claim. Binary sizes are build results, not runtime RAM or
CPU measurements. Generated disposable-key firmware remains local and must not
be distributed as a generic image.

Logs for this session: `/tmp/streamoo-audit-{tests,tests-final,safety,cppcheck,clang,build,build-final,build-network,browser,layout}.log`,
`/tmp/streamoo-audit-repro-{json,icy}.log`,
`/tmp/streamoo-audit-fuzz-{build,playlist,hls,icy,stations,wifi,boundaries}.log`,
`/tmp/streamoo-audit-afl-{build,run}.log` and `/tmp/streamoo-audit-cjson.su`.
The fresh optional-dependency run is recorded in
`/tmp/streamoo-audit-{host-deps,modern-tests}.log`.
These are temporary local evidence, not tracked deliverables or permanent links.
Reproduction commands and the important diagnostic evidence are retained here.

## Security assessment

The fixed JSON stack-budget problem is the main availability issue found.
NUL handling now enforces intended schemas; empty ICY feeds avoid UB. Existing
TLS validation, HTTPS downgrade refusal, request-size limits, custom-header
CSRF protection, CSP and retained configuration on failed updates were preserved.
No new memory-corruption crash was found by the short fuzz/static campaigns.
No remote-code-execution or credential-disclosure exploit was demonstrated.

The open HTTP panel remains unsuitable for hostile LANs. Browser-origin checks
do not authorize direct clients; DNS rebinding needs a separate design/test.
Shared AP credentials and unencrypted physical storage are explicit deployment
risks. No authentication framework, insecure TLS fallback, automatic formatting
or security eFuse change was added. GitHub private reporting/security scanning
still need owner action. Short host campaigns cannot certify third-party
binary decoders, SDK HTTP/TLS framing or target concurrency.

## Performance assessment

Measured here: successful builds, binary sizes and host test/campaign execution
counts only. No new ESP32-S3 CPU utilization, throughput, heap fragmentation,
ISR latency, Wi-Fi jitter or audio-underrun measurements were made. The linear
JSON scan runs only on small configuration requests; the ICY early return has
no extra work for audio payloads. No performance gain is asserted.

Possible future work: measure DSP overlap-copy cost before considering a ring
layout; compare decoder/core/load profiles and network reserve behavior under
the same station fixtures. These are unverified optimization hypotheses.
Preserve aligned SIMD work areas, internal ISR buffers and pre-volume DSP tap.

## Release readiness and GitHub metadata

The repository is appropriate for an explicitly **development source release**,
with detailed build/license notices and clearly disclosed acceptance limits.
It already has `v0.1.0` at the baseline commit, published 2026-11-14, not marked
prerelease, with source archives and no binary assets. Do not overwrite it or
claim these local fixes are in that tag. After review, approved publication and
CI confirmation, **v0.1.1** is a reasonable next patch version. No new tag created.

Metadata inspection commands executed:

```sh
git status --short
git branch --show-current
git log -5 --oneline
gh repo view PROTONMM/StreamOO --json name,description,defaultBranchRef,hasIssuesEnabled,repositoryTopics,latestRelease
gh api repos/PROTONMM/StreamOO/tags
gh api repos/PROTONMM/StreamOO/actions/workflows
gh api repos/PROTONMM/StreamOO/community/profile
gh api repos/PROTONMM/StreamOO
gh api repos/PROTONMM/StreamOO/private-vulnerability-reporting
gh release view v0.1.0 --repo PROTONMM/StreamOO --json assets,body,isDraft,isPrerelease,tagName
```

Recommended settings commands below were **not executed**. Issues are already
enabled, so no change is necessary. If intentionally disabled later, the owner
can restore them with `gh repo edit PROTONMM/StreamOO --enable-issues`.

```sh
gh repo edit PROTONMM/StreamOO --description 'Modular ESP-IDF Internet radio for ESP32-S3 with SSD1322 OLED, PCM5102A, DSP visualizations and a local web player'
gh repo edit PROTONMM/StreamOO --add-topic esp32-s3 --add-topic esp-idf --add-topic internet-radio --add-topic embedded --add-topic audio --add-topic dsp --add-topic ssd1322
gh api --method PUT repos/PROTONMM/StreamOO/private-vulnerability-reporting
gh api --method PUT repos/PROTONMM/StreamOO/automated-security-fixes
gh api --method PATCH repos/PROTONMM/StreamOO --input - <<'JSON'
{"security_and_analysis":{"secret_scanning":{"status":"enabled"},"secret_scanning_push_protection":{"status":"enabled"}}}
JSON
```

Feature availability/permissions remain GitHub-dependent. Dependabot security
fixes do not replace audits of ESP Component Registry locks or vendored codecs.

## Remaining work, in priority order

1. Review and merge the parser fixes through the normal approved publication
   process; execute GitHub CI and resolve any container-specific differences.
2. On S3 test malformed/deep JSON without reset, normal panel setup/playback,
   rapid station/STOP/reconnect changes, DMA continuity, stack margins and a
   multi-hour soak across supported codecs. Log exact firmware/configuration.
3. Complete interrupted-write/full-storage, corrupt NVS/catalog and physical
   power-cycle protocols on real flash. Host fault stubs are not power tests.
4. Decide whether deployment remains trusted LAN only; design/test Host or
   authentication defenses if untrusted networks/browser rebinding are in scope.
5. Enable private reporting and secret protection if approved; add Topics and
   a useful description. Run longer seeded fuzz campaigns; AFL needs suitable
   system crash handling. Never bypass sanitizer/crash checks to claim a pass.
6. Stress diagnostic task snapshot lifetimes and measure target SIMD/decoder
   performance before changing scheduling, buffers, copies or optimization.

Suggested reviewable commit groups (not committed):

- `fix(portal): bound JSON recursion and reject decoded NUL fields`
- `fix(streaming): handle empty ICY feeds without undefined behavior`
- `ci: build ESP-IDF firmware and run host safety and parser fuzz checks`
- `docs: document audit evidence, security scope and contributor workflow`
