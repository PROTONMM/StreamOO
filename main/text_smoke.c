#include "text_smoke.h"
#include "streamoo_font5x8.h"
#include <string.h>

bool streamoo_text_smoke(uint8_t *frame, size_t bytes)
{
    streamoo_canvas_t canvas;
    if (!streamoo_canvas_init(&canvas, frame, bytes, 256, 64, 128)) return false;
    streamoo_canvas_fill(&canvas, (streamoo_rect_t){0, 0, 256, 64}, 0);
    const char *lines[] = {
        "StreamOO - UTF-8 PL", "Bitmap 5x8 / cell 6x8",
        "ąćęłńóśźż", "ĄĆĘŁŃÓŚŹŻ", "Zazolc gesla jazn",
        "Zażółć gęślą jaźń", "ąĄ ćĆ ęĘ łŁ ńŃ óÓ śŚ źŹ żŻ", "ogonki: ą Ą ę Ę",
    };
    for (unsigned row = 0; row < sizeof(lines) / sizeof(lines[0]); ++row) {
        if (!streamoo_text_draw(&canvas, 0, row * 8, (const uint8_t *)lines[row],
                                 strlen(lines[row]), 15, streamoo_font5x8, NULL)) return false;
    }
    return true;
}
