# 06 — Audio Pipeline and Playback

[English index](00_INDEX.md)

## Formats and output

The current adapters support MP3, AAC-LC/HE-AAC/HE-AACv2 in the supported ADTS family,
native FLAC and Ogg Vorbis/Opus. The UI's AAC label does not identify every AAC
profile. M3U/PLS URLs are resolved before decoding; HLS has its own restricted
segment reader. See [format validation](FORMAT_VALIDATION.md) for boundaries.

Live decoded PCM accepts mono/stereo at 8–48 kHz with supported 16/24/32-bit
sample formats. The production path converts/resamples to the fixed output:
48 kHz, signed stereo 32-bit PCM and Philips I²S 32-bit slots, no MCLK.
Do not confuse a diagnostic fixture adapter's narrower format with the live path.

The default FLAC backend is dr_flac 0.13.4. Kconfig also exposes micro-flac 0.3.0
and the Espressif 2.6.2 backend. They share transport and output policies but
have different decoder behavior and acceptance histories. Using dr_flac does
not remove the other Espressif codecs/parsers or their license terms.

The pinned Espressif 2.6.2 decoder audit confirms partial ESP32-S3 SIMD in
the Opus SILK LPC filter (`opus_silk_lpc_analysis_core`), called during decoding
and packet loss concealment. Its use depends on the Opus mode. Audited hot paths
in MP3, AAC/HE-AAC, Espressif FLAC and Vorbis did not show confirmed SIMD arithmetic.
The default dr_flac backend uses scalar C on this target; micro-flac uses scalar
Xtensa assembly for LPC. These findings do not imply vectorization of all Opus
or automatic codec acceleration from the separate spectrum FFT. Detailed evidence
is retained in the [Polish SIMD audit](../CODEC_SIMD_AUDIT.md).

## Ownership and states

The player alone owns playback sessions. Web and encoder actions enqueue copied
commands. Actual phases in `streamoo_player_phase_t` are:

| Phase | Meaning |
| --- | --- |
| `STOPPED` | Playback not requested |
| `WAIT_NETWORK` | Playback requested but usable STA connectivity is unavailable |
| `CONNECTING` | Opening the source/session |
| `BUFFERING` | Preparing or rebuilding audio reserve |
| `PLAYING` | Normal playback |
| `RETRY` | Waiting to reconnect after a session failure |

The public `running` flag records playback intent; it is not proof that samples
are currently audible. Errors are reported separately. Retry backoff grows through
1, 2, 4, 8 and up to 15 seconds. A failed source does not cause automatic selection
of a different station.

STOP ends the source and fades output; PLAY starts a new connection. There is no
timeshift. Changing station creates a new generation, clears old metadata/buffers
and cancels previous work. Choosing the same already-playing ID does not reconnect.
Catalog edits preserve IDs; a new URL takes effect at the next connection, while
removing the active ID stops playback.

## Buffering and real-time output

```text
network reader → compressed PSRAM ring → parser/decoder → live conversion
→ internal PCM FIFO → pre-gain measurement tap → gain ramp → I²S DMA → DAC
```

The default compressed ring is 256 KiB and does not overwrite unread data.
Full input buffers apply backpressure. DATA/WAIT/END/ERROR distinguish source
conditions; WAIT preserves partial PCM instead of treating starvation as EOF.
Reader and decoder run independently. Large live decoder scratch memory is in
PSRAM; internal PCM is allocated before HTTP/TLS startup to protect a contiguous
allocation from later fragmentation.

PCM blocks contain 240 stereo frames: five milliseconds at 48 kHz. The 32-block
output ring holds 160 ms. Its reserve policy starts fading near ten queued blocks
(50 ms) and resumes after at least 24 blocks (120 ms). A short underrun does not
itself restart the TCP session. Compressed FLAC startup/refill reserves are separate
from this PCM policy and depend on stream information; larger buffers alone are
not evidence of a resolved delivery problem.

The I²S callback performs bounded work using internal/IRAM-safe data. It must not
allocate, access flash, compute FFT, wait on a mutex, or perform blocking I/O.
The measurement tap drops samples when full instead of blocking audio.

## Volume

The accepted gain curve is `(volume / 100)^2`, implemented with fixed-point values.
Volume 0 mutes and 100 is unity gain. Changes use an approximately 50 ms sample-domain
ramp. Measurements occur before gain, so the spectrum and meters remain independent
of the volume slider. Volume preferences are saved after five seconds without changes.

## Bitrate and errors

CBR and VBR sources are supported within the codec/container boundaries.
No per-codec maximum playback bitrate has been established by a complete hardware
acceptance matrix. Generated fixtures use 128 kb/s CBR MP3, a 128 kb/s AAC-LC target
and a 96 kb/s Opus target; these are test inputs, not decoder limits. The
512000 bit/s HLS limit filters declared `EXT-X-STREAM-INF` `BANDWIDTH` values,
not all direct streams. Likewise, the 4096 kb/s metadata/estimate plausibility
limit is not a certified playback capability.

Positive ICY bitrate and the selected HLS variant's declared bitrate provide
source information. When unavailable, decoded-audio time and compressed-byte
accounting provide an estimate, separate from HTTP delivery speed. The web UI marks
estimates with ≈ and detected variation with “Zmienny” (Variable). Variation detection
needs several observation windows; the label is not a bitstream-wide VBR guarantee.

FLAC input parsing validates frame structure and CRC; resynchronization requires
validated following frames. A parser rejection, decoder failure and reader/TLS error
are different failure classes. Collect their reason, generation, buffer counts and
timing together. A log-only test cannot establish clean audio. See
[diagnostic procedures](12_TESTS_DELIVERY.md) and [known limitations](13_DECISIONS_OPEN.md).
