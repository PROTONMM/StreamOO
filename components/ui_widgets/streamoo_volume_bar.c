#include "streamoo_volume_bar.h"
#include <limits.h>
bool streamoo_volume_bar_draw(streamoo_canvas_t *c, streamoo_rect_t r,
                              int volume, uint8_t gray) {
  if (!c || !c->pixels || volume < 0 || volume > 100 || gray > 15 ||
      r.width < 3 || r.height < 3 || r.x == INT16_MAX || r.y == INT16_MAX)
    return false;
  streamoo_canvas_fill(c, r, gray);
  streamoo_rect_t inner = {(int16_t)(r.x + 1), (int16_t)(r.y + 1),
                           (int16_t)(r.width - 2), (int16_t)(r.height - 2)};
  streamoo_canvas_fill(c, inner, 0);
  inner.width = (int16_t)(((int32_t)inner.width * volume + 50) / 100);
  streamoo_canvas_fill(c, inner, gray);
  return true;
}
