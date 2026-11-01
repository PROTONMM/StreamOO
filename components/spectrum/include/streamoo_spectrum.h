#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif
#define STREAMOO_SPECTRUM_MAX 2048u
#define STREAMOO_SPECTRUM_BANDS 24u
/* Task-only spectrum backend, NOT I2S/decoder callback.
 * The ESP32-S3 build uses ESP-DSP sc16 AES3 SIMD when configured; host tests
 * retain the portable float reference implementation.
 * Init precomputes symmetric Hann N-1 and twiddles. N=1024 or2048.
 * SIMD workspace must be 16-byte aligned. Its read-only twiddle table has
 * firmware lifetime; one task owns all init calls and ESP-DSP sc16 state.
 * Analyze performs no allocation, I/O, locks or table initialization.
 * Input is signed32 stereo before gain at explicit sample_rate (8..48kHz).
 * Per-channel Hann-weighted DC mean removed, then FFT. Stereo combines POWER so channels
 * in opposite phase cannot cancel. Bins hold peak amplitude relative to FS,
 * coherent-gain corrected; Nyquist is not doubled, DC reported as0.
 * 24 approximately logarithmic bands cover ALL bins1..N/2 exactly once,
 * with at least one bin per band; edges are exclusive on the right.
 * Band value is the maximum bin amplitude, linear Q15/saturated32767.
 * No dB mapping/smoothing or overlap collector in this core.
 * Single task owner, init/analyze never concurrent. Invalid calls don't write. */
typedef struct {
    uint32_t size, sample_rate;
    float window_sum;
    float window[STREAMOO_SPECTRUM_MAX];
#if defined(ESP_PLATFORM) && CONFIG_STREAMOO_DSP_ENABLE_SIMD
    _Alignas(16) int16_t work[STREAMOO_SPECTRUM_MAX*2];
#else
    float twiddle[STREAMOO_SPECTRUM_MAX];
    float work[STREAMOO_SPECTRUM_MAX*2];
#endif
    float amplitude[STREAMOO_SPECTRUM_MAX/2+1];
    uint16_t edges[STREAMOO_SPECTRUM_BANDS+1];
    uint16_t bands[STREAMOO_SPECTRUM_BANDS];
} streamoo_spectrum_t;
bool streamoo_spectrum_init(streamoo_spectrum_t *, uint32_t size, uint32_t sample_rate);
bool streamoo_spectrum_analyze(streamoo_spectrum_t *, const int32_t *lr, size_t frames);
const char *streamoo_spectrum_backend(void);
