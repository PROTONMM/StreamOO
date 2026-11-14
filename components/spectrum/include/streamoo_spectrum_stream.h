#pragma once
#include "streamoo_spectrum.h"
#define STREAMOO_SPECTRUM_LIVE_SIZE 1024u
/* Task-owned fixed48kHz collector,50% overlap, no allocation or I/O.
 * Feed one sequenced block1..240frames. Reset on gap/duplicate, uint32 wrap
 * continuous. calculate=false retains overlap but skips expensive transforms.
 * With240<512 at most one transform per call. Result true only on new FFT.
 * Reset the collector (not coefficients) on selection/session changes. */
typedef struct {
    streamoo_spectrum_t fft;
    int32_t lr[STREAMOO_SPECTRUM_LIVE_SIZE*2];
    uint32_t filled, previous_sequence, windows, skipped, gaps;
    bool seen;
} streamoo_spectrum_stream_t;
bool streamoo_spectrum_stream_init(streamoo_spectrum_stream_t *);
void streamoo_spectrum_stream_reset(streamoo_spectrum_stream_t *);
bool streamoo_spectrum_stream_feed(streamoo_spectrum_stream_t *, const int32_t *,
                                   size_t frames, uint32_t sequence, bool calculate);
/* Shared presentation: -45..0dBFS, gamma2, tone peak amplitude reference. */
uint16_t streamoo_spectrum_display_level(uint16_t linear_q15);
