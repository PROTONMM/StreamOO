#pragma once
#include "streamoo_text.h"

typedef enum { STREAMOO_TEXT_LEFT, STREAMOO_TEXT_CENTER, STREAMOO_TEXT_RIGHT } streamoo_text_align_t;

/* Stateless ping-pong marquee: pause at start, move, pause at end, return.
 * Positions depend only on elapsed time; skipped frames do not change speed.
 * speed=0 disables movement. Reset elapsed to zero when text/station changes.
 * Times are milliseconds, speed is pixels per second. */
uint32_t streamoo_marquee_offset(uint32_t content_width, uint32_t viewport_width,
                                 uint64_t elapsed_ms, uint32_t speed, uint32_t pause_ms);

/* Single line, nominal ink width = six-pixel advances minus last gap.
 * Fitting text follows alignment; overflow follows marquee starting at left.
 * Text is clipped to bounds and inherited widget limits, including vertically.
 * glyph/context are the native 5x8 provider; no allocations or RTOS calls. */
bool streamoo_text_line(streamoo_canvas_t *canvas, streamoo_rect_t bounds,
                         const uint8_t *text, size_t bytes, streamoo_text_align_t align,
                         uint64_t elapsed_ms, uint32_t speed, uint32_t pause_ms,
                         uint8_t gray, streamoo_glyph_fn glyph, void *context);
