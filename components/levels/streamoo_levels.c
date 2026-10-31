#include "streamoo_levels.h"
#include <string.h>

/* Exact floor sqrt via bounded binary search: mean square <=32768^2. */
static uint16_t amplitude(uint64_t square) {
    uint32_t low = 0, high = 32769;
    while (high - low > 1) {
        uint32_t mid = low + (high - low) / 2;
        if ((uint64_t)mid * mid <= square) low = mid;
        else high = mid;
    }
    return low > 32767 ? 32767 : (uint16_t)low;
}
bool streamoo_levels_init(streamoo_levels_t *s, uint32_t frames) {
    if (!s || !frames || frames > STREAMOO_LEVELS_WINDOW_MAX) return false;
    *s = (streamoo_levels_t){.window_frames = frames};
    return true;
}
bool streamoo_levels_process(streamoo_levels_t *s, const int32_t *lr,
                             size_t frames, uint32_t sequence) {
    if (!s || !lr || !frames || frames > STREAMOO_LEVELS_BLOCK_MAX ||
        !s->window_frames || s->window_frames > STREAMOO_LEVELS_WINDOW_MAX)
        return false;
    if (s->have_sequence && sequence != s->previous_sequence + UINT32_C(1)) {
        memset(s->squares, 0, sizeof s->squares);
        memset(s->peak, 0, sizeof s->peak);
        s->frames = 0;
        s->result.valid = false;
        memset(s->result.rms, 0, sizeof s->result.rms);
        memset(s->result.peak, 0, sizeof s->result.peak);
        ++s->result.discontinuities;
    }
    s->have_sequence = true;
    s->previous_sequence = sequence;
    for (size_t i = 0; i < frames; ++i) {
        for (unsigned ch = 0; ch < 2; ++ch) {
            int32_t sample = lr[2 * i + ch] / 65536;
            uint32_t magnitude = sample < 0 ? (uint32_t)-sample : (uint32_t)sample;
            s->squares[ch] += (uint64_t)magnitude * magnitude;
            if (magnitude > s->peak[ch]) s->peak[ch] = magnitude;
        }
        if (++s->frames == s->window_frames) {
            for (unsigned ch = 0; ch < 2; ++ch) {
                s->result.rms[ch] = amplitude(s->squares[ch] / s->window_frames);
                s->result.peak[ch] = s->peak[ch] > 32767 ? 32767 : (uint16_t)s->peak[ch];
                s->squares[ch] = 0;
                s->peak[ch] = 0;
            }
            s->frames = 0;
            s->result.valid = true;
            ++s->result.windows;
        }
    }
    return true;
}
