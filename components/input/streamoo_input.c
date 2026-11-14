#include "streamoo_input.h"
#include <stddef.h>

void streamoo_input_init(streamoo_input_t *s, bool a, bool b, bool pressed,
                         bool half, bool reversed, uint64_t now)
{
    if (!s) return;
    *s = (streamoo_input_t){.previous_ab = (a ? 2 : 0) | (b ? 1 : 0),
        .candidate = pressed, .pressed = pressed, .armed = !pressed,
        .candidate_since = now, .pressed_since = now,
        .half = half, .reversed = reversed};
}

streamoo_input_event_t streamoo_input_sample(streamoo_input_t *s, bool a, bool b,
                                             bool pressed, uint64_t now)
{
    streamoo_input_event_t event = {0};
    if (!s) return event;
    if (pressed != s->candidate) {
        s->candidate = pressed;
        s->candidate_since = now;

    }
    if (s->candidate != s->pressed && now - s->candidate_since >= 20) {
        s->pressed = s->candidate;
        if (s->pressed) {
            s->pressed_since = now;
        } else {
            if (s->armed && !s->suppress_click)
                event.kind = now - s->pressed_since >= 600 ?
                    STREAMOO_INPUT_LONG : STREAMOO_INPUT_SHORT;
            s->armed = true;
            s->suppress_click = false;
        }
    }
    if (!s->candidate && !s->pressed && now - s->candidate_since >= 20)
        s->suppress_click = false;
    const uint8_t ab = (a ? 2 : 0) | (b ? 1 : 0);
    const uint8_t old = s->previous_ab;
    s->previous_ab = ab;
    if ((old ^ ab) == 3) {
        s->quarter_steps = 0;
        return event;
    }
    /* Positive sequence: 11 -> 01 -> 00 -> 10 -> 11. Own truth table. */
    static const int8_t steps[16] = {0,-1,1,0, 1,0,0,-1, -1,0,0,1, 0,1,-1,0};
    s->quarter_steps += steps[old * 4 + ab];
    const int threshold = s->half ? 2 : 4;
    if ((ab == 3 || (s->half && ab == 0)) &&
        (s->quarter_steps >= threshold || s->quarter_steps <= -threshold)) {
        /* A rotation on a release sample takes precedence over a click. Raw
         * candidate press also suppresses accidental volume before debounce. */
        event.delta = s->quarter_steps > 0 ? 1 : -1;
        if (s->reversed) event.delta = -event.delta;
        event.kind = (s->pressed || s->candidate) ?
            STREAMOO_INPUT_PRESSED_ROTATE : STREAMOO_INPUT_ROTATE;
        if (s->pressed || s->candidate) s->suppress_click = true;
        s->quarter_steps = 0;
    } else if (ab == 3 || (s->half && ab == 0)) {
        s->quarter_steps = 0;
    }
    return event;
}
