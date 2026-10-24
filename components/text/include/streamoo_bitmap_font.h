#pragma once
#include "streamoo_text_layout.h"

/* Row-aligned alpha 4bpp, high nibble first. x/y relative to pen/baseline. */
typedef struct {
    uint32_t cp, offset;
    int16_t x, y, advance;
    uint16_t w, h;
} streamoo_glyph_t;
typedef struct {
    const streamoo_glyph_t *glyphs;
    const uint8_t *bitmap;
    uint32_t count, bitmap_bytes;
    uint16_t line_height;
    int16_t ascent;
} streamoo_font_t;

const streamoo_glyph_t *streamoo_font_lookup(const streamoo_font_t *font, uint32_t codepoint);
/* Invalid font/overflow returns SIZE_MAX; missing glyphs resolve to ?. */
size_t streamoo_bitmap_text_width(const streamoo_font_t *font, const uint8_t *text, size_t bytes);
bool streamoo_bitmap_text_line(streamoo_canvas_t *canvas, streamoo_rect_t bounds,
                               const streamoo_font_t *font, const uint8_t *text, size_t bytes,
                               streamoo_text_align_t align, uint64_t elapsed_ms,
                               uint32_t speed, uint32_t pause_ms, uint8_t gray);
/* StreamOO UI16: OFL bitmap subset derived from unmodified Krypton source. */
const streamoo_font_t *streamoo_ui_font(void);
