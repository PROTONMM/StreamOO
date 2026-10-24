#pragma once

#include "streamoo_canvas.h"

/* Single-line 5x8 font contract; provider writes five column bytes, bit0 top.
 * Return false for unsupported Unicode. Renderer draws an original box fallback.
 * No font tables or Arduino-compatible byte translation are embedded here. */
typedef bool (*streamoo_glyph_fn)(void *context, uint32_t codepoint, uint8_t columns[5]);

/* Strict UTF-8; malformed input produces U+FFFD and consumes one byte.
 * Empty input consumes zero. Embedded NUL is data, not a terminator. */
uint32_t streamoo_utf8_next(const uint8_t *text, size_t bytes, size_t *consumed);
/* Advance is six pixels per Unicode scalar/replacement, including trailing gap.
 * Saturates at SIZE_MAX on overflow. Never allocates. */
size_t streamoo_text_width(const uint8_t *text, size_t bytes);
/* Draw exactly bytes in a single 8-pixel row; control characters render as spaces.
 * Coordinates are surface-relative; the canvas owns clipping. No allocation.
 * Source text must not overlap the destination framebuffer. */
bool streamoo_text_draw(streamoo_canvas_t *canvas, int16_t x, int16_t y,
                         const uint8_t *text, size_t bytes, uint8_t gray,
                         streamoo_glyph_fn glyph, void *context);

/* Extended positioning used by marquee: signed origin minus unsigned pixel
 * offset is computed wide, avoiding int16 wrap for long text. */
bool streamoo_text_draw_scrolled(streamoo_canvas_t *canvas, int32_t x, int32_t y,
                                  uint32_t offset, const uint8_t *text, size_t bytes,
                                  uint8_t gray, streamoo_glyph_fn glyph, void *context);
