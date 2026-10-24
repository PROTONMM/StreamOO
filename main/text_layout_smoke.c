#include "text_layout_smoke.h"
#include "streamoo_font5x8.h"
#include "streamoo_text_layout.h"
#include <string.h>

bool streamoo_text_layout_smoke(uint8_t *frame, size_t bytes, uint64_t elapsed_ms)
{
    streamoo_canvas_t canvas;
    if (!streamoo_canvas_init(&canvas, frame, bytes, 256, 64, 128)) return false;
    streamoo_canvas_fill(&canvas, (streamoo_rect_t){0, 0, 256, 64}, 0);
    const struct { streamoo_rect_t bounds; const char *text; streamoo_text_align_t align; } lines[] = {
        {{0, 0, 256, 16}, "StreamOO - test tekstu", STREAMOO_TEXT_CENTER},
        {{0, 16, 256, 8}, "Długi tytuł: Zażółć gęślą jaźń - StreamOO radio", STREAMOO_TEXT_LEFT},
        {{0, 24, 256, 8}, "Opis: polskie znaki i przewijanie bez skoków - test", STREAMOO_TEXT_LEFT},
        {{0, 40, 80, 8}, "LEWO", STREAMOO_TEXT_LEFT},
        {{80, 40, 96, 8}, "ŚRODEK", STREAMOO_TEXT_CENTER},
        {{176, 40, 80, 8}, "PRAWO", STREAMOO_TEXT_RIGHT},
    };
    for (unsigned i = 0; i < sizeof(lines) / sizeof(lines[0]); ++i) {
        if (!streamoo_text_line(&canvas, lines[i].bounds, (const uint8_t *)lines[i].text,
                                 strlen(lines[i].text), lines[i].align, elapsed_ms,
                                 30, 1000, 15, streamoo_font5x8, NULL)) return false;
    }
    return true;
}
