#include "display_timing_probe.h"
#include "streamoo_font5x8.h"

bool streamoo_oled_probe_draw(uint8_t *frame, size_t bytes,
                               streamoo_oled_probe_phase_t phase, uint32_t elapsed_ms)
{
    if (phase < STREAMOO_OLED_HOLD || phase >= STREAMOO_OLED_PROBE_COUNT) return false;
    streamoo_canvas_t canvas;
    if (!streamoo_canvas_init(&canvas, frame, bytes, 256, 64, 128)) return false;
    streamoo_canvas_fill(&canvas, (streamoo_rect_t){0, 0, 256, 64}, 0);
    static const uint8_t title[] = "StreamOO OLED TEST";
    static const uint8_t note[] = "GS6 / GS8 / GS10 / GS15 - patrz UART";
    if (!streamoo_text_draw(&canvas, 0, 0, title, sizeof title - 1, 15, streamoo_font5x8, NULL) ||
        !streamoo_text_draw(&canvas, 0, 8, note, sizeof note - 1, 15, streamoo_font5x8, NULL)) return false;
    const uint8_t reference[] = {6, 8, 10, 15};
    for (unsigned i = 0; i < 4; ++i)
        streamoo_canvas_fill(&canvas, (streamoo_rect_t){i * 64, 20, 48, 6}, reference[i]);
    const uint8_t gray[] = {8, 10, 15};
    unsigned triangle = (elapsed_ms % 2000) / 40;
    if (triangle > 25) triangle = 50 - triangle;
    for (unsigned i = 0; i < 3; ++i) {
        int16_t height = 16, offset = 0;
        if (phase == STREAMOO_OLED_LOAD25 || phase == STREAMOO_OLED_LOAD10) height = triangle;
        if (phase == STREAMOO_OLED_MOVE25) { height = 24; offset = triangle; }
        streamoo_canvas_fill(&canvas, (streamoo_rect_t){i * 80 + offset, 63 - height, 48, height}, gray[i]);
    }
    return true;
}
