#pragma once
#include "streamoo_canvas.h"

#define STREAMOO_VIS_MAX_BANDS 64
#define STREAMOO_VIS_LEVEL_MAX 32767

typedef enum {
    STREAMOO_VIS_SPECTRUM,
    STREAMOO_VIS_BARGRAPH,
    STREAMOO_VIS_ANALOG,
} streamoo_visualization_kind_t;

/* Immutable display snapshot, normalized unsigned levels 0..32767.
 * The producer owns calibration, log-frequency aggregation, smoothing and
 * peak hold. These are display levels, not uncalibrated PCM samples/dB.
 * No FFT, sample queue or volume gain belongs in this renderer. */
typedef struct {
    uint16_t rms[2];
    uint16_t peak[2];
    uint16_t bands[STREAMOO_VIS_MAX_BANDS];
    uint8_t band_count;
} streamoo_visualization_model_t;

/* Bounds must be inside the physical canvas, at least 8x8. Inherited clip
 * still applies. Clears and draws only bounds; never modifies model.
 * Caller supplies an immutable snapshot for this call and serializes canvas
 * ownership. Invalid inputs fail before writes. No allocation/global state. */
bool streamoo_visualization_draw(streamoo_canvas_t *canvas, streamoo_rect_t bounds,
                                  streamoo_visualization_kind_t kind,
                                  const streamoo_visualization_model_t *model);
