/* Bounds/overflow stress test for the actual renderer, no peripheral stubs. */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "streamoo_canvas.h"
#include "canvas_smoke.h"

static uint32_t state = 0x12345678;
static uint32_t next(void)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

int main(void)
{
    uint8_t guarded[8194];
    memset(guarded, 0xa5, sizeof(guarded));
    streamoo_canvas_t canvas;
    assert(streamoo_canvas_init(&canvas, guarded + 1, 8192, 256, 64, 128));
    const uint8_t mask[] = {0x55, 0xaa, 0xff, 0x00};
    for (unsigned i = 0; i < 10000; ++i) {
        streamoo_rect_t rect = {(int16_t)next(), (int16_t)next(),
                                (int16_t)next(), (int16_t)next()};
        streamoo_canvas_t child;
        assert(streamoo_canvas_child(&canvas, rect, &child));
        const uint8_t gray = next() & 31;
        streamoo_canvas_fill(&child, rect, gray);
        streamoo_canvas_outline(&canvas, rect, gray);
        streamoo_canvas_pixel(&canvas, rect.x, rect.y, gray);
        streamoo_canvas_mask(&canvas, rect.x, rect.y, 16, 2, mask, sizeof(mask), 2, gray);
        if ((i & 255) == 0)
            streamoo_canvas_line(&canvas, rect.x, rect.y, rect.width, rect.height, gray);
        assert(guarded[0] == 0xa5 && guarded[8193] == 0xa5);
    }
    assert(streamoo_canvas_smoke(guarded + 1, 8192));
    assert(guarded[0] == 0xa5 && guarded[8193] == 0xa5);
    return 0;
}
