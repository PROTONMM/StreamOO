#include "streamoo_canvas.h"

#include <stdlib.h>
#include <string.h>

static int32_t minimum(int32_t a, int32_t b) { return a < b ? a : b; }
static int32_t maximum(int32_t a, int32_t b) { return a > b ? a : b; }

static streamoo_rect_t intersect(streamoo_rect_t a, streamoo_rect_t b)
{
    if (a.width <= 0 || a.height <= 0 || b.width <= 0 || b.height <= 0)
        return (streamoo_rect_t){0};
    int32_t x = maximum(a.x, b.x), y = maximum(a.y, b.y);
    int32_t right = minimum((int32_t)a.x + a.width, (int32_t)b.x + b.width);
    int32_t bottom = minimum((int32_t)a.y + a.height, (int32_t)b.y + b.height);
    if (right <= x || bottom <= y) return (streamoo_rect_t){0};
    return (streamoo_rect_t){x, y, right - x, bottom - y};
}

bool streamoo_canvas_init(streamoo_canvas_t *c, uint8_t *pixels,
                           size_t bytes, int16_t width, int16_t height, uint16_t stride)
{
    if (!c) return false;
    *c = (streamoo_canvas_t){0};
    if (!pixels || width <= 0 || height <= 0 || stride < ((unsigned)width + 1) / 2 ||
        bytes < (size_t)stride * height) return false;
    *c = (streamoo_canvas_t){pixels, bytes, width, height, stride,
                           {0, 0, width, height}, {0, 0, width, height}};
    return true;
}

void streamoo_canvas_clip(streamoo_canvas_t *c, streamoo_rect_t bounds)
{
    if (!c || !c->pixels) return;
    c->clip = intersect(c->bounds, bounds);
}

bool streamoo_canvas_child(const streamoo_canvas_t *parent, streamoo_rect_t bounds,
                            streamoo_canvas_t *child)
{
    if (!parent || !child || !parent->pixels) return false;
    *child = *parent;
    child->clip = intersect(parent->clip, bounds);
    child->bounds = child->clip;
    return true;
}

static void pixel(streamoo_canvas_t *c, int32_t x, int32_t y, uint8_t gray)
{
    if (x < c->clip.x || y < c->clip.y ||
        x >= (int32_t)c->clip.x + c->clip.width ||
        y >= (int32_t)c->clip.y + c->clip.height) return;
    uint8_t *byte = c->pixels + (size_t)y * c->stride + x / 2;
    if (x & 1) *byte = (*byte & 0xf0) | gray;
    else *byte = (*byte & 0x0f) | (gray << 4);
}

void streamoo_canvas_pixel(streamoo_canvas_t *c, int16_t x, int16_t y, uint8_t gray)
{
    if (!c || !c->pixels || gray > 15) return;
    pixel(c, x, y, gray);
}

/* Use wide coordinates internally, so right/bottom edges never wrap int16_t. */
static void fill(streamoo_canvas_t *c, int32_t x, int32_t y,
                 int32_t width, int32_t height, uint8_t gray)
{
    int32_t left = maximum(x, c->clip.x), top = maximum(y, c->clip.y);
    int32_t right = minimum(x + width, (int32_t)c->clip.x + c->clip.width);
    int32_t bottom = minimum(y + height, (int32_t)c->clip.y + c->clip.height);
    if (width <= 0 || height <= 0 || left >= right || top >= bottom) return;
    for (int32_t row = top; row < bottom; ++row) {
        int32_t start = left, end = right;
        if (start & 1) pixel(c, start++, row, gray);
        if (end & 1) pixel(c, --end, row, gray);
        if (start < end) memset(c->pixels + (size_t)row * c->stride + start / 2,
                                gray * 17, (size_t)(end - start) / 2);
    }
}

void streamoo_canvas_fill(streamoo_canvas_t *c, streamoo_rect_t rect, uint8_t gray)
{
    if (!c || !c->pixels || gray > 15) return;
    fill(c, rect.x, rect.y, rect.width, rect.height, gray);
}

void streamoo_canvas_outline(streamoo_canvas_t *c, streamoo_rect_t r, uint8_t gray)
{
    if (!c || !c->pixels || gray > 15 || r.width <= 0 || r.height <= 0) return;
    fill(c, r.x, r.y, r.width, 1, gray);
    fill(c, r.x, (int32_t)r.y + r.height - 1, r.width, 1, gray);
    fill(c, r.x, r.y, 1, r.height, gray);
    fill(c, (int32_t)r.x + r.width - 1, r.y, 1, r.height, gray);
}

void streamoo_canvas_line(streamoo_canvas_t *c, int16_t ax, int16_t ay,
                           int16_t bx, int16_t by, uint8_t gray)
{
    if (!c || !c->pixels || gray > 15 || c->clip.width <= 0 || c->clip.height <= 0) return;
    int32_t x = ax, y = ay;
    if (maximum(ax, bx) < c->clip.x || maximum(ay, by) < c->clip.y ||
        minimum(ax, bx) >= (int32_t)c->clip.x + c->clip.width ||
        minimum(ay, by) >= (int32_t)c->clip.y + c->clip.height) return;
    const int32_t dx = abs((int32_t)bx - ax), dy = -abs((int32_t)by - ay);
    const int32_t sx = ax < bx ? 1 : -1, sy = ay < by ? 1 : -1;
    int32_t error = dx + dy;
    for (;;) {
        pixel(c, x, y, gray);
        if (x == bx && y == by) break;
        const int32_t twice = error * 2;
        if (twice >= dy) { error += dy; x += sx; }
        if (twice <= dx) { error += dx; y += sy; }
    }
}

bool streamoo_canvas_mask(streamoo_canvas_t *c, int16_t x, int16_t y,
                           int16_t width, int16_t height, const uint8_t *mask,
                           size_t bytes, uint16_t stride, uint8_t gray)
{
    if (!c || !c->pixels || !mask || width <= 0 || height <= 0 || gray > 15 ||
        stride < ((unsigned)width + 7) / 8 || bytes < (size_t)stride * height) return false;
    /* Intersect before scanning the bitmap: work scales with visible area. */
    const streamoo_rect_t visible = intersect(c->clip, (streamoo_rect_t){x, y, width, height});
    for (int32_t py = visible.y; py < (int32_t)visible.y + visible.height; ++py) {
        for (int32_t px = visible.x; px < (int32_t)visible.x + visible.width; ++px) {
            const unsigned sx = px - x;
            if (mask[(size_t)(py - y) * stride + sx / 8] & (0x80 >> (sx & 7)))
                pixel(c, px, py, gray);
        }
    }
    return true;
}
