#include "canvas_smoke.h"
#include "streamoo_canvas.h"

bool streamoo_canvas_smoke(uint8_t *frame, size_t bytes)
{
    streamoo_canvas_t canvas;
    if (!streamoo_canvas_init(&canvas, frame, bytes, 256, 64, 128)) return false;
    streamoo_canvas_fill(&canvas, (streamoo_rect_t){0, 0, 256, 64}, 0);
    const streamoo_rect_t panels[] = {
        {0, 0, 256, 16}, {0, 16, 256, 8}, {0, 24, 256, 8},
        {0, 32, 44, 32}, {44, 32, 132, 32}, {176, 32, 80, 32},
    };
    for (unsigned i = 0; i < sizeof(panels) / sizeof(panels[0]); ++i) {
        streamoo_canvas_t child;
        streamoo_canvas_child(&canvas, panels[i], &child);
        streamoo_canvas_outline(&child, panels[i], 8);
        const int16_t y = panels[i].y + panels[i].height / 2;
        /* Intentionally exceed widget bounds to demonstrate isolation. */
        streamoo_canvas_line(&child, -30, y, 280, y, 4);
    }
    streamoo_canvas_t center;
    streamoo_canvas_child(&canvas, panels[4], &center);
    streamoo_canvas_line(&center, 44, 32, 175, 63, 15);
    streamoo_canvas_line(&center, 44, 63, 175, 32, 15);
    const uint8_t arrow[] = {0x20, 0x30, 0xf8, 0xfc, 0xf8, 0x30, 0x20};
    streamoo_canvas_mask(&canvas, 124, 4, 6, 7, arrow, sizeof(arrow), 1, 15);
    return true;
}
