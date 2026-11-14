#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum { STREAMOO_INPUT_NONE, STREAMOO_INPUT_ROTATE,
    STREAMOO_INPUT_SHORT, STREAMOO_INPUT_LONG, STREAMOO_INPUT_PRESSED_ROTATE
} streamoo_input_kind_t;
typedef struct { streamoo_input_kind_t kind; int delta; } streamoo_input_event_t;
typedef struct {
    uint64_t candidate_since, pressed_since;
    uint8_t previous_ab;
    int8_t quarter_steps;
    bool candidate, pressed, suppress_click, armed, half, reversed;
} streamoo_input_t;
/* Active-low button normalized to pressed=true. Single owner, monotonic ms.
 * Initial held button must be released before clicks are armed. Half detents
 * at 00/11; full detents at 11. Invalid two-bit jumps discard partial motion.
 * No allocation, platform calls or I/O. Sample fast enough to observe edges. */
void streamoo_input_init(streamoo_input_t *s, bool a, bool b, bool pressed,
                         bool half, bool reversed, uint64_t now_ms);
streamoo_input_event_t streamoo_input_sample(streamoo_input_t *s, bool a, bool b,
                                             bool pressed, uint64_t now_ms);
