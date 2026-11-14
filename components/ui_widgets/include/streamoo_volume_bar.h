#pragma once
#include "streamoo_canvas.h"
/* Control volume 0..100, not an audio meter. Outline and filled portion use
 * gray; interior is cleared. Respects inherited clipping; no alloc/I/O. */
bool streamoo_volume_bar_draw(streamoo_canvas_t *, streamoo_rect_t, int volume,
                              uint8_t gray);
