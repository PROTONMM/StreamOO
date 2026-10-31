#include "streamoo_visualization.h"

static uint32_t scale(uint16_t value, uint16_t extent)
{
    return (uint32_t)value * extent / STREAMOO_VIS_LEVEL_MAX;
}

static void spectrum(streamoo_canvas_t *canvas, streamoo_rect_t b,
                     const streamoo_visualization_model_t *model)
{
    for (unsigned i = 0; i < model->band_count; ++i) {
        const int32_t x = b.x + (uint32_t)i * b.width / model->band_count;
        const int32_t next = b.x + (uint32_t)(i + 1) * b.width / model->band_count;
        const int16_t width = next - x > 1 ? next - x - 1 : 1;
        const int16_t height = scale(model->bands[i], b.height - 1);
        if (height) {
            const int16_t top = b.y + b.height - 1 - height;
            streamoo_canvas_fill(canvas, (streamoo_rect_t){x, top, width, height}, 8);
            streamoo_canvas_fill(canvas, (streamoo_rect_t){x, top, width, 1}, 15);
        }
    }
    streamoo_canvas_line(canvas, b.x, b.y + b.height - 1,
                         b.x + b.width - 1, b.y + b.height - 1, 4);
}

static void bargraph(streamoo_canvas_t *canvas, streamoo_rect_t b,
                     const streamoo_visualization_model_t *model)
{
    const int16_t lane = b.height / 2;
    for (unsigned channel = 0; channel < 2; ++channel) {
        const int16_t y = b.y + channel * lane + 1;
        const int16_t width = scale(model->rms[channel], b.width);
        streamoo_canvas_fill(canvas, (streamoo_rect_t){b.x, y, width, lane - 2}, 10);
        if (model->peak[channel]) {
            const int16_t x = b.x + scale(model->peak[channel], b.width - 1);
            streamoo_canvas_line(canvas, x, y, x, y + lane - 3, 15);
        }
    }
}

/* Original integer geometry for a 120-degree meter. Piecewise interpolation
 * avoids runtime trig/float; it is a display scale, not a calibrated VU law. */
static const int16_t dx[] = {-87, -71, -50, -26, 0, 26, 50, 71, 87};
static const int16_t dy[] = {-50, -71, -87, -97, -100, -97, -87, -71, -50};

typedef struct { int16_t x, y; } point_t;

static point_t meter_point(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint16_t value)
{
    const uint32_t phase = (uint32_t)value * 8;
    const unsigned segment = phase / STREAMOO_VIS_LEVEL_MAX;
    const unsigned fraction = phase % STREAMOO_VIS_LEVEL_MAX;
    int32_t x = dx[segment], y = dy[segment];
    if (segment < 8) {
        x += (dx[segment + 1] - dx[segment]) * (int32_t)fraction / STREAMOO_VIS_LEVEL_MAX;
        y += (dy[segment + 1] - dy[segment]) * (int32_t)fraction / STREAMOO_VIS_LEVEL_MAX;
    }
    return (point_t){cx + x * rx / 100, cy + y * ry / 100};
}

static void analog(streamoo_canvas_t *canvas, streamoo_rect_t b,
                   const streamoo_visualization_model_t *model)
{
    for (unsigned channel = 0; channel < 2; ++channel) {
        const int16_t left = b.x + channel * (b.width / 2);
        const int16_t width = channel ? b.width - b.width / 2 : b.width / 2;
        const int16_t cx = left + width / 2, cy = b.y + b.height - 1;
        const int16_t rx = width / 2 - 1, ry = b.height - 2;
        point_t previous = {0};
        for (unsigned tick = 0; tick <= 8; ++tick) {
            point_t p = {cx + dx[tick] * rx / 100, cy + dy[tick] * ry / 100};
            if (tick) streamoo_canvas_line(canvas, previous.x, previous.y, p.x, p.y, 4);
            streamoo_canvas_pixel(canvas, p.x, p.y, 8);
            previous = p;
        }
        const point_t needle = meter_point(cx, cy, rx, ry, model->rms[channel]);
        streamoo_canvas_line(canvas, cx, cy, needle.x, needle.y, 15);
        if (model->peak[channel]) {
            const point_t peak = meter_point(cx, cy, rx, ry, model->peak[channel]);
            streamoo_canvas_pixel(canvas, peak.x, peak.y, 15);
        }
    }
}

bool streamoo_visualization_draw(streamoo_canvas_t *canvas, streamoo_rect_t b,
                                  streamoo_visualization_kind_t kind,
                                  const streamoo_visualization_model_t *model)
{
    if (!canvas || !canvas->pixels || !model || kind < STREAMOO_VIS_SPECTRUM ||
        kind > STREAMOO_VIS_ANALOG || b.x < 0 || b.y < 0 || b.width < 8 || b.height < 8 ||
        (int32_t)b.x + b.width > canvas->width || (int32_t)b.y + b.height > canvas->height ||
        model->band_count > STREAMOO_VIS_MAX_BANDS) return false;
    for (unsigned channel = 0; channel < 2; ++channel) {
        if (model->rms[channel] > STREAMOO_VIS_LEVEL_MAX ||
            model->peak[channel] > STREAMOO_VIS_LEVEL_MAX) return false;
    }
    for (unsigned i = 0; i < model->band_count; ++i) {
        if (model->bands[i] > STREAMOO_VIS_LEVEL_MAX) return false;
    }
    streamoo_canvas_t child;
    if (!streamoo_canvas_child(canvas, b, &child)) return false;
    streamoo_canvas_fill(&child, b, 0);
    switch (kind) {
    case STREAMOO_VIS_SPECTRUM: spectrum(&child, b, model); break;
    case STREAMOO_VIS_BARGRAPH: bargraph(&child, b, model); break;
    case STREAMOO_VIS_ANALOG: analog(&child, b, model); break;
    }
    return true;
}
