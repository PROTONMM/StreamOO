#include "streamoo_ui_connection_row.h"
#include "streamoo_font5x8.h"
#include "streamoo_text_layout.h"
#include "streamoo_volume_bar.h"
#include <stdio.h>
bool streamoo_ui_connection_row_draw(streamoo_canvas_t *canvas,
                                     const uint8_t *ip, size_t bytes,
                                     int volume) {
  if (!canvas || !ip || !bytes || bytes > 15 || volume < 0 || volume > 100)
    return false;
  for (size_t i = 0; i < bytes; ++i)
    if ((ip[i] < '0' || ip[i] > '9') && ip[i] != '.')
      return false;
  char address[20], level[8];
  int a = snprintf(address, sizeof address, "IP: %.*s", (int)bytes,
                   (const char *)ip);
  int v = snprintf(level, sizeof level, "VOL %d", volume);
  if (a < 0 || (size_t)a >= sizeof address || v < 0 ||
      (size_t)v >= sizeof level)
    return false;
  streamoo_canvas_t row;
  if (!streamoo_canvas_child(canvas, (streamoo_rect_t){0, 24, 256, 8}, &row))
    return false;
  streamoo_canvas_fill(&row, (streamoo_rect_t){0, 24, 256, 8}, 0);
  return streamoo_text_line(&row, (streamoo_rect_t){0, 24, 128, 8},
                            (const uint8_t *)address, (size_t)a,
                            STREAMOO_TEXT_LEFT, 0, 0, 0, 6, streamoo_font5x8,
                            NULL) &&
         streamoo_text_line(&row, (streamoo_rect_t){128, 24, 44, 8},
                            (const uint8_t *)level, (size_t)v,
                            STREAMOO_TEXT_LEFT, 0, 0, 0, 15, streamoo_font5x8,
                            NULL) &&
         streamoo_volume_bar_draw(&row, (streamoo_rect_t){176, 25, 80, 6},
                                  volume, 15);
}
