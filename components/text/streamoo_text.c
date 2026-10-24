#include "streamoo_text.h"

#include <limits.h>
#include <string.h>

uint32_t streamoo_utf8_next(const uint8_t *text, size_t bytes, size_t *consumed)
{
    if (consumed) *consumed = 0;
    if (!text || !bytes) return 0xfffd;
    if (consumed) *consumed = 1;
    const uint8_t first = text[0];
    if (first < 0x80) return first;
    unsigned count;
    uint32_t value, minimum;
    if (first >= 0xc2 && first <= 0xdf) {
        count = 2; value = first & 0x1f; minimum = 0x80;
    } else if (first >= 0xe0 && first <= 0xef) {
        count = 3; value = first & 0x0f; minimum = 0x800;
    } else if (first >= 0xf0 && first <= 0xf4) {
        count = 4; value = first & 7; minimum = 0x10000;
    } else return 0xfffd;
    if (bytes < count) return 0xfffd;
    for (unsigned i = 1; i < count; ++i) {
        if ((text[i] & 0xc0) != 0x80) return 0xfffd;
        value = (value << 6) | (text[i] & 0x3f);
    }
    if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
        return 0xfffd;
    if (consumed) *consumed = count;
    return value;
}

size_t streamoo_text_width(const uint8_t *text, size_t bytes)
{
    if (!text) return 0;
    size_t width = 0;
    while (bytes) {
        size_t consumed;
        streamoo_utf8_next(text, bytes, &consumed);
        if (width > SIZE_MAX - 6) return SIZE_MAX;
        width += 6;
        text += consumed;
        bytes -= consumed;
    }
    return width;
}

bool streamoo_text_draw(streamoo_canvas_t *canvas, int16_t x, int16_t y,
                         const uint8_t *text, size_t bytes, uint8_t gray,
                         streamoo_glyph_fn glyph, void *context)
{
    if (!canvas || !canvas->pixels || (!text && bytes) || !glyph || gray > 15)
        return false;
    if (canvas->clip.width <= 0 || canvas->clip.height <= 0 ||
        (int32_t)y + 8 <= canvas->clip.y ||
        y >= (int32_t)canvas->clip.y + canvas->clip.height) return true;
    int32_t cursor = x;
    const int32_t right = (int32_t)canvas->clip.x + canvas->clip.width;
    while (bytes && cursor < right) {
        size_t consumed;
        uint32_t codepoint = streamoo_utf8_next(text, bytes, &consumed);
        text += consumed;
        bytes -= consumed;
        if (cursor + 5 > canvas->clip.x) {
            uint8_t columns[5] = {0};
            if (codepoint < 0x20 || codepoint == 0x7f) codepoint = 0x20;
            if (codepoint != 0x20 && !glyph(context, codepoint, columns)) {
                const uint8_t fallback[] = {0xff, 0x81, 0x81, 0x81, 0xff};
                memcpy(columns, fallback, sizeof(columns));
            }
            uint8_t rows[8] = {0};
            for (unsigned row = 0; row < 8; ++row) {
                for (unsigned col = 0; col < 5; ++col) {
                    if (columns[col] & (1u << row)) rows[row] |= 0x80 >> col;
                }
            }
            /* cursor < right <= INT16_MAX, starts as int16; no narrowing wrap. */
            streamoo_canvas_mask(canvas, (int16_t)cursor, y, 5, 8, rows, sizeof(rows), 1, gray);
        }
        cursor += 6;
    }
    return true;
}
