#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Original synthetic scene: fixed reference text/gray fields above bars.
 * HOLD and REPEAT are byte-identical. LOAD25/LOAD10 differ only in cadence.
 * MOVE25 changes position while preserving the lit area at every gray level. */
typedef enum {
    STREAMOO_OLED_HOLD,
    STREAMOO_OLED_REPEAT,
    STREAMOO_OLED_LOAD25,
    STREAMOO_OLED_LOAD10,
    STREAMOO_OLED_MOVE25,
    STREAMOO_OLED_PROBE_COUNT
} streamoo_oled_probe_phase_t;

bool streamoo_oled_probe_draw(uint8_t *frame, size_t bytes,
                               streamoo_oled_probe_phase_t phase, uint32_t elapsed_ms);
