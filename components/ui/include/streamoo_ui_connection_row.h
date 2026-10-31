#pragma once
#include "streamoo_canvas.h"
/* Fixed third HOME row y24..31: left dim IP, bright VOL and horizontal bar. */
bool streamoo_ui_connection_row_draw(streamoo_canvas_t *, const uint8_t *ip,
                                     size_t bytes, int volume);
