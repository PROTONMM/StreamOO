# 12 — Testing, Diagnostics and Delivery

[English index](00_INDEX.md)

## Automated validation

Run from the repository root:

```sh
python3 tools/check_config.py sdkconfig --platform
python3 -m unittest discover -s tests -v
idf.py build
git diff --check
```

Use the active ESP-IDF 6.1 environment for the build. Host tests require Python and
a native compiler; individual groups also use Node.js, NumPy or FFmpeg tools.
Review SKIP results instead of counting them as passed evidence. The test scripts
specify their own prerequisites. No host test proves electrical behavior on the board.

Optional frontend integration and layout checks:

```sh
python3 tools/check_portal_browser.py
python3 tools/check_portal_layout.py
```

They use a synthetic local server and `/usr/bin/google-chrome`. They exercise UI
flows and layouts without contacting the radio or public streams. A simulated mobile
viewport is not a test of physical Android touch behavior.

Focused numerical comparison:

```sh
python3 -m unittest discover -s tests -p test_spectrum.py
python3 -m unittest discover -s tests -p test_spectrum_sc16.py
```

NumPy supplies an independent FFT reference. The host sc16 reference does not
benchmark the ESP32-S3 instruction set. For test-to-feature mapping, see
[14_TRACEABILITY.md](14_TRACEABILITY.md).

## Hardware acceptance checklist

| Area | Procedure and evidence |
| --- | --- |
| Boot and pins | Confirm N16R8, cold boot/reset, display orientation, grayscale, encoder direction and DAC channels |
| Setup | Start with no usable saved network, connect to AP, import stations, save Wi-Fi, observe STA/IP and AP shutdown |
| Recovery | Try a failing profile before a valid one; submit an invalid replacement and verify previous settings recover |
| Playback | Listen to each supported format, check correct rate/channels, PLAY/STOP, rapid station changes and retry |
| UI | Confirm four views, peak markers, readable fonts, list timeout, no old metadata after station changes |
| Web | Test PC and physical phone, theme retention, full-list search, filtered previous/next and volume while dragging |
| Catalog | Export, edit, append import, duplicates, conflict from two browsers, active-station removal and reboot persistence |
| Clock and discovery | Time zone/DST, HTTPS after boot, `.local` and IP access, custom hostname and collision behavior |
| Persistence | Wait for writes, power-cycle and verify; separately test interrupted writes with recoverable test data |
| Load and reliability | Long playback, Wi-Fi interruptions, TLS/HLS, simultaneous web editing and switching; log heap/stack/underruns |

Do not mark a listening test passed when only logs were inspected. Do not describe
an unexecuted checklist as evidence. Keep the firmware revision, build profile,
stream/fixture type, duration, observations and unresolved items with each result.

## Diagnostic profile

Normal radio builds keep periodic detailed UART output disabled. Enable
`STREAMOO_DIAGNOSTICS` or use the [separate diagnostic build](BUILD_GUIDE.md#diagnostics-and-tests)
to collect task CPU, heap/stack, PCM/decoder/reader and metering information.
Hardware tests are separately opt-in; they are not needed to diagnose real streams.

For dropouts, capture a coherent interval around the event: station ID and stream
generation, reader error/status, delivery rate, decoder timing/reason, PCM reserve,
underrun/restart counters, minimum/largest internal memory and stack high-water marks.
An HTTP 200 does not prove that codec input was complete or valid. A parser CRC
failure does not, by itself, establish its upstream cause.

Keep logs private until names, URLs, tokens and network details have been reviewed.
Do not include the AP configuration JSON or generated secret header. Retain the
matching ELF for crash decoding.

## Controlled PCM stress test

Enable hardware tests, disable networking, keep UI enabled and select the controlled
audio source. In the I2S test menu set:

```text
CONFIG_STREAMOO_PCM_STALL_TEST=y
CONFIG_STREAMOO_PCM_TAP_DELAY_MS=100
```

After the first PLAY, the producer pauses for 400 ms approximately three seconds
later. Expect one controlled interruption with fades and automatic recovery. The
deliberately delayed measurement consumer should cause tap drops, not continuing
audio interruptions. Observe counters and listen. This tests output isolation, not
Internet reliability or every DMA fault.

After testing, restore stall=n, tap delay=0, disable hardware tests/diagnostics and
enable network radio. Do not save stress options as normal product defaults.

## Delivery contents

Provide the firmware/build revision, intended hardware and pin profile, relevant
`sdkconfig` without secrets, instructions for flashing, the user guide, known
limitations, test results and applicable license notices. Retain the matching ELF
for support. Do not distribute private credentials or the owner's station catalog.

The root MIT license covers original work, while the combined image includes
third-party terms. Follow [LICENSING.md](LICENSING.md) and the actual dependency
versions in the build. “Final documentation” describes the current product; it does
not certify that all open release acceptance items have been completed.
