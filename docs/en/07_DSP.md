# 07 — FFT, RMS, Peak and Display Mapping

[English index](00_INDEX.md)

Audio analysis runs in a separate metering task using a droppable pre-gain tap.
It must never delay PCM output. Sequence gaps and session changes invalidate/reset
analysis rather than combining unrelated samples. FFT is requested only when the
active home visualization requires it; RMS/peak analysis is independent.

## Spectrum processing

The normal size is N=1024, with N=2048 available through Kconfig. A collector uses
50% overlap and bounded analysis cadence. The symmetric Hann window is:

```text
w[n] = 0.5 × (1 − cos(2πn / (N − 1))), 0 ≤ n < N
```

DC removal is Hann-weighted. Window normalization and backend scaling preserve
the dBFS interpretation. Stereo power is combined before taking the maximum
FFT-bin amplitude in each of 24 logarithmic bands. Both spectrum views use these
same band values. The removed experimental RMS-band aggregation is not used.

The default backend is ESP-DSP 1.8.2 sc16 AES3 SIMD for ESP32-S3, with aligned
workspace and scaling compensation. Disabling `STREAMOO_DSP_ENABLE_SIMD` selects
the scalar reference path. Host reference tests compare numerical results; they
do not execute or benchmark ESP32-S3 vector instructions. A controlled speedup
measurement against float under identical conditions remains separate work.

Vectorization applies to the FFT kernel's complex multiplication and butterfly
operations. Hann/DC processing, block scaling, bit reversal and band aggregation
remain scalar. “AES3” in the FFT backend name is independent of the hardware AES
encryption peripheral used by the TLS stack.

## Spectrum display

The display maps −45…0 dBFS into a clamped 0–1 range, then squares it (gamma 2)
before mapping it to pixel height. This reduces low-level background visually;
it does not change the FFT or turn the display into a calibrated energy integral.

Both views use a stationary 1×1-pixel checkerboard fill, grayscale 10 against black.
Bar attack is immediate; full-scale bar decay is 320 ms. In the peak-hold view,
bright horizontal markers at grayscale 15 hold for 500 ms after a new/equal nonzero
peak, then fall through a full scale in 2000 ms. A marker never falls below its bar.
Each band has independent timing. New sessions/reset/expiry clear old peaks.

## Stereo meters

Bargraphs and needles display RMS levels with separate peak indicators. Their
time-domain analysis and ballistics are unchanged by the removal of spectrum RMS.
Needle peak markers occupy two vertical pixels: one on the scale and one immediately
below it. Volume changes do not change these readings because the tap precedes gain.

## Validity and load

The analysis snapshot has a timestamp and generation. Analysis may expire after
200 ms without a fresh FFT. For short gaps, the home renderer can decay the last
valid model for up to 800 ms only for the same station/session/visualization.
This visual bridge does not create new measurements or renew the peak hold timer.
STOP, changed station/view or longer gaps clear it.

Keep frame drops distinct from audio underruns. Under load, dropping visualization
blocks is intentional; repeatedly starving audio is not. Record FFT duration,
tap gaps, task CPU use and PCM reserve together. Do not infer codec SIMD acceleration
from the fact that the spectrum uses ESP-DSP.

## Validation

Test silence, DC, impulses, on-bin and off-bin tones, stereo input, normalization,
clipping, backend agreement, gaps and peak-hold timing. NumPy is the independent
FFT reference in host tests. Missing NumPy means skipped comparison tests, not
passed comparisons. Physical display contrast, metering scheduling and long-running
audio under concurrent web activity require board tests.
