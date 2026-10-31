#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define STREAMOO_SPECTRUM_MAX 2048u
#define STREAMOO_SPECTRUM_BANDS 24u
/* Portable float reference backend; task-only, NOT I2S/decoder callback.
 * Caller-owned workspace (about37KiB), no allocation/global state/I/O.
 * Init precomputes symmetric Hann N-1 and twiddles. N=1024 or2048.
 * Input is signed32 stereo before gain at explicit sample_rate (8..48kHz).
 * Per-channel DC mean removed, then FFT. Stereo combines POWER so channels
 * in opposite phase cannot cancel. Bins hold peak amplitude relative to FS,
 * coherent-gain corrected; Nyquist is not doubled, DC reported as0.
 * 24 approximately logarithmic bands cover ALL bins1..N/2 exactly once,
 * with at least one bin per band; edges are exclusive on the right.
 * Band value is the maximum bin amplitude, linear Q15/saturated32767.
 * No dB mapping/smoothing, overlap collector, SIMD or live publication yet.
 * Single task owner, init/analyze never concurrent. Invalid calls don't write. */
typedef struct {
    uint32_t size, sample_rate;
    float window_sum;
    float window[STREAMOO_SPECTRUM_MAX];
    float twiddle[STREAMOO_SPECTRUM_MAX];
    float work[STREAMOO_SPECTRUM_MAX*2];
    float amplitude[STREAMOO_SPECTRUM_MAX/2+1];
    uint16_t edges[STREAMOO_SPECTRUM_BANDS+1];
    uint16_t bands[STREAMOO_SPECTRUM_BANDS];
} streamoo_spectrum_t;
bool streamoo_spectrum_init(streamoo_spectrum_t *, uint32_t size, uint32_t sample_rate);
bool streamoo_spectrum_analyze(streamoo_spectrum_t *, const int32_t *lr, size_t frames);
