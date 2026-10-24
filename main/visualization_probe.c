#include "visualization_probe.h"
#include "streamoo_visualization.h"
#include "streamoo_text_layout.h"
#include "streamoo_font5x8.h"
#include <string.h>

static uint16_t triangle(uint32_t phase)
{
    phase %= 4000;
    return (phase < 2000 ? phase : 4000 - phase) * 32767 / 2000;
}

bool streamoo_visualization_probe(streamoo_canvas_t *canvas, uint64_t elapsed)
{
    const uint32_t phase = elapsed % 4000;
    streamoo_visualization_model_t model = {.band_count = 24};
    model.rms[0] = triangle(phase);
    model.rms[1] = triangle(phase + 1000);
    for (unsigned channel = 0; channel < 2; ++channel) {
        const uint32_t peak = (uint32_t)model.rms[channel] + 8192;
        model.peak[channel] = peak > 32767 ? 32767 : peak;
    }
    for (unsigned band = 0; band < model.band_count; ++band)
        model.bands[band] = triangle(phase + band * 110);
    const streamoo_visualization_kind_t kind = (elapsed / 6000) % 3;
    if (!streamoo_visualization_draw(canvas, (streamoo_rect_t){44, 32, 132, 32}, kind, &model))
        return false;
    const char *name[] = {"FFT", "L/R", "VU"};
    return streamoo_text_line(canvas, (streamoo_rect_t){0, 40, 44, 8},
                              (const uint8_t *)"TEST", 4, STREAMOO_TEXT_LEFT,
                              0, 0, 0, 12, streamoo_font5x8, NULL) &&
           streamoo_text_line(canvas, (streamoo_rect_t){176, 40, 80, 8},
                              (const uint8_t *)name[kind], strlen(name[kind]), STREAMOO_TEXT_RIGHT,
                              0, 0, 0, 15, streamoo_font5x8, NULL);
}
