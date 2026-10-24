#include "streamoo_bitmap_font.h"
#include <limits.h>

const streamoo_glyph_t *streamoo_font_lookup(const streamoo_font_t *font, uint32_t code)
{
    if (!font || !font->glyphs || !font->bitmap || !font->count) return NULL;
    size_t left = 0, right = font->count;
    while (left < right) {
        size_t mid = left + (right - left) / 2;
        if (font->glyphs[mid].cp < code) left = mid + 1;
        else right = mid;
    }
    if (left == font->count || font->glyphs[left].cp != code) return NULL;
    return &font->glyphs[left];
}

static const streamoo_glyph_t *resolve(const streamoo_font_t *font, uint32_t code)
{
    if (code < 0x20 || code == 0x7f) code = ' ';
    const streamoo_glyph_t *g = streamoo_font_lookup(font, code);
    return g ? g : streamoo_font_lookup(font, '?');
}

static bool valid(const streamoo_font_t *font, const streamoo_glyph_t *g)
{
    if (!g || g->advance <= 0 || g->w > INT16_MAX || g->h > INT16_MAX) return false;
    const size_t bytes = ((size_t)g->w + 1) / 2 * g->h;
    return g->offset <= font->bitmap_bytes && bytes <= font->bitmap_bytes - g->offset;
}

size_t streamoo_bitmap_text_width(const streamoo_font_t *font, const uint8_t *text, size_t bytes)
{
    if (!font || !font->glyphs || !font->bitmap || !font->count || !font->line_height ||
        font->ascent < 0 || font->ascent > font->line_height || (!text && bytes)) return SIZE_MAX;
    size_t width = 0;
    while (bytes) {
        size_t consumed;
        const streamoo_glyph_t *g = resolve(font, streamoo_utf8_next(text, bytes, &consumed));
        if (!valid(font, g) || width > SIZE_MAX - g->advance) return SIZE_MAX;
        width += g->advance;
        text += consumed; bytes -= consumed;
    }
    return width;
}

bool streamoo_bitmap_text_line(streamoo_canvas_t *canvas, streamoo_rect_t bounds,
                               const streamoo_font_t *font, const uint8_t *text, size_t bytes,
                               streamoo_text_align_t align, uint64_t elapsed,
                               uint32_t speed, uint32_t pause, uint8_t gray)
{
    if (!canvas || !canvas->pixels || !font || (!text && bytes) || gray > 15 ||
        align < STREAMOO_TEXT_LEFT || align > STREAMOO_TEXT_RIGHT) return false;
    const size_t width = streamoo_bitmap_text_width(font, text, bytes);
    if (width == SIZE_MAX || width > UINT32_MAX) return false;
    if (bounds.width <= 0 || bounds.height <= 0) return true;
    streamoo_canvas_t child;
    if (!streamoo_canvas_child(canvas, bounds, &child)) return false;
    int64_t pen = bounds.x;
    if (width <= (unsigned)bounds.width) {
        const int32_t slack = bounds.width - width;
        if (align == STREAMOO_TEXT_CENTER) pen += slack / 2;
        if (align == STREAMOO_TEXT_RIGHT) pen += slack;
    } else pen -= streamoo_marquee_offset(width, bounds.width, elapsed, speed, pause);
    const int32_t baseline = bounds.y + font->ascent +
        (bounds.height > font->line_height ? (bounds.height - font->line_height) / 2 : 0);
    while (bytes) {
        size_t consumed;
        const streamoo_glyph_t *g = resolve(font, streamoo_utf8_next(text, bytes, &consumed));
        text += consumed; bytes -= consumed;
        /* width validated all source metrics before the first write. */
        const int64_t x = pen + g->x;
        const int32_t y = baseline + g->y;
        if (g->w && g->h && x < (int32_t)child.clip.x + child.clip.width &&
            x + g->w > child.clip.x && y < (int32_t)child.clip.y + child.clip.height &&
            y + g->h > child.clip.y) {
            if (x < INT16_MIN || x > INT16_MAX || y < INT16_MIN || y > INT16_MAX) return false;
            const uint16_t stride = (g->w + 1) / 2;
            if (!streamoo_canvas_alpha4(&child, x, y, g->w, g->h, font->bitmap + g->offset,
                                         (size_t)stride * g->h, stride, gray)) return false;
        }
        pen += g->advance;
    }
    return true;
}
