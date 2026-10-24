#include "text_layout_smoke.h"
#include "streamoo_font5x8.h"
#include "streamoo_text_layout.h"
#include "streamoo_bitmap_font.h"
#include "visualization_probe.h"
#include <string.h>

bool streamoo_text_layout_smoke(uint8_t *frame, size_t bytes, uint64_t elapsed_ms)
{
    streamoo_canvas_t canvas;
    if (!streamoo_canvas_init(&canvas, frame, bytes, 256, 64, 128)) return false;
    streamoo_canvas_fill(&canvas, (streamoo_rect_t){0, 0, 256, 64}, 0);
    const struct { streamoo_rect_t bounds; const char *text; streamoo_text_align_t align; } lines[] = {
        {{0, 16, 256, 8}, "Długi tytuł: Zażółć gęślą jaźń - StreamOO radio", STREAMOO_TEXT_LEFT},
        {{0, 24, 256, 8}, "Opis: polskie znaki i przewijanie bez skoków - test", STREAMOO_TEXT_LEFT},
    };
    const char *station = "StreamOO - Zażółć gęślą jaźń";
    if (!streamoo_bitmap_text_line(&canvas, (streamoo_rect_t){0, 0, 256, 16},
                                    streamoo_ui_font(), (const uint8_t *)station, strlen(station),
                                    STREAMOO_TEXT_CENTER, elapsed_ms, 30, 1000, 15)) return false;
    for (unsigned i = 0; i < sizeof(lines) / sizeof(lines[0]); ++i) {
        if (!streamoo_text_line(&canvas, lines[i].bounds, (const uint8_t *)lines[i].text,
                                 strlen(lines[i].text), lines[i].align, elapsed_ms,
                                 30, 1000, 15, streamoo_font5x8, NULL)) return false;
    }
    return streamoo_visualization_probe(&canvas, elapsed_ms);
}
