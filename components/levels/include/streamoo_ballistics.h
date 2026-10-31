#pragma once
#include "streamoo_levels.h"
/* Presentation only:10Hz, RMS attack160ms/release500ms, peak hold400ms
 * then release800ms. Time-based, Q16 internal precision, no allocation/I/O. */
typedef struct {
    uint32_t rms_q16[2], peak_q16[2];
    uint16_t target_rms[2], pending_peak[2];
    int64_t last_us, hold_until_us[2];
} streamoo_ballistics_t;
void streamoo_ballistics_init(streamoo_ballistics_t *, int64_t now_us);
void streamoo_ballistics_feed(streamoo_ballistics_t *, const streamoo_levels_result_t *);
bool streamoo_ballistics_update(streamoo_ballistics_t *, int64_t now_us,
                                uint16_t rms[2], uint16_t peak[2]);
