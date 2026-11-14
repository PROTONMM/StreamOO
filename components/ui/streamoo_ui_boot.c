#include "streamoo_ui_boot.h"
#include "streamoo_font5x8.h"
#include "streamoo_text_layout.h"
#include "streamoo_boot_wordmark.inc"
#include <string.h>

bool streamoo_ui_boot_draw(uint8_t *frame, size_t bytes) {
  if (!frame || bytes != 8192)
    return false;
  memset(frame, 0, bytes);
  streamoo_canvas_t canvas;
  if (!streamoo_canvas_init(&canvas, frame, bytes, 256, 64, 128) ||
      !streamoo_canvas_alpha4(&canvas, 16, 6, 224, 40, boot_wordmark,
                              sizeof boot_wordmark, 112, 15))
    return false;
  static const uint8_t label[] = "I N T E R N E T   R A D I O";
  return streamoo_text_line(&canvas, (streamoo_rect_t){0, 53, 256, 8},
                            label, sizeof label - 1, STREAMOO_TEXT_CENTER, 0, 0,
                            0, 7, streamoo_font5x8, NULL);
}
