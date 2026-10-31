#pragma once
#include "streamoo_spectrum.h"
/* Presentation only, AFTER dB mapping. Immediate attack; falling edge limited
 * to one full scale per320ms. Feed only new valid spectra with monotonic time.
 * This is not FFT/window averaging and never renews measurement timestamps.
 * Reset on session/widget change. No allocation, I/O or global state. */
typedef struct {
    uint16_t bands[STREAMOO_SPECTRUM_BANDS];
    int64_t last_us;
} streamoo_spectrum_envelope_t;
void streamoo_spectrum_envelope_init(streamoo_spectrum_envelope_t *, int64_t now_us);
void streamoo_spectrum_envelope_feed(streamoo_spectrum_envelope_t *,
                                     const uint16_t target[STREAMOO_SPECTRUM_BANDS],
                                     int64_t now_us);
