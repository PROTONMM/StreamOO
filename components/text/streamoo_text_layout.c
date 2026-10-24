#include "streamoo_text_layout.h"

uint32_t streamoo_marquee_offset(uint32_t content, uint32_t viewport,
                                 uint64_t elapsed, uint32_t speed, uint32_t pause)
{
    if (content <= viewport || !viewport || !speed) return 0;
    const uint32_t distance = content - viewport;
    const uint64_t travel = ((uint64_t)distance * 1000 + speed - 1) / speed;
    const uint64_t cycle = 2 * ((uint64_t)pause + travel);
    uint64_t phase = elapsed % cycle;
    if (phase < pause) return 0;
    phase -= pause;
    if (phase < travel) return (uint64_t)phase * speed / 1000;
    phase -= travel;
    if (phase < pause) return distance;
    phase -= pause;
    return distance - (uint64_t)phase * speed / 1000;
}

bool streamoo_text_line(streamoo_canvas_t *canvas, streamoo_rect_t bounds,
                         const uint8_t *text, size_t bytes, streamoo_text_align_t align,
                         uint64_t elapsed, uint32_t speed, uint32_t pause,
                         uint8_t gray, streamoo_glyph_fn glyph, void *context)
{
    if (!canvas || !canvas->pixels || (!text && bytes) || !glyph || gray > 15 ||
        align < STREAMOO_TEXT_LEFT || align > STREAMOO_TEXT_RIGHT) return false;
    if (bounds.width <= 0 || bounds.height <= 0) return true;
    size_t width = streamoo_text_width(text, bytes);
    if (width) --width;
    if (width > UINT32_MAX) return false;
    streamoo_canvas_t child;
    if (!streamoo_canvas_child(canvas, bounds, &child)) return false;
    int32_t x = bounds.x;
    uint32_t offset = 0;
    if (width <= (unsigned)bounds.width) {
        const int32_t slack = bounds.width - width;
        if (align == STREAMOO_TEXT_CENTER) x += slack / 2;
        if (align == STREAMOO_TEXT_RIGHT) x += slack;
    } else {
        offset = streamoo_marquee_offset(width, bounds.width, elapsed, speed, pause);
    }
    const int32_t y = bounds.y + (bounds.height > 8 ? (bounds.height - 8) / 2 : 0);
    return streamoo_text_draw_scrolled(&child, x, y, offset, text, bytes, gray, glyph, context);
}
