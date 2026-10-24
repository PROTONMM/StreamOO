#pragma once
#include "streamoo_canvas.h"

/* Explicit synthetic stimulus for the display diagnostic, not an audio source. */
bool streamoo_visualization_probe(streamoo_canvas_t *canvas, uint64_t elapsed_ms);
