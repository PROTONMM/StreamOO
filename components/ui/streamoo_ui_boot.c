#include "streamoo_ui_boot.h"
#include "streamoo_bitmap_font.h"
#include "streamoo_font5x8.h"
#include <string.h>
static uint8_t ring(int x, int y, int cx) {
  int dx = x - cx, dy = y - 30, d = dx * dx + dy * dy;
  return d >= 100 && d <= 144  ? 15
         : d >= 81 && d < 100  ? 6
         : d > 144 && d <= 169 ? 4
                               : 0;
}
bool streamoo_ui_boot_draw(uint8_t *frame, size_t bytes) {
  if (!frame || bytes != 8192)
    return false;
  memset(frame, 0, bytes);
  /* Use the first 1024 bytes as a temporary 128x16 canvas. Scaled wordmark
   * is written below it, so no second framebuffer or large stack buffer. */
  streamoo_canvas_t small, canvas;
  if (!streamoo_canvas_init(&small, frame, 1024, 128, 16, 64) ||
      !streamoo_canvas_init(&canvas, frame, bytes, 256, 64, 128))
    return false;
  const uint8_t name[] = "StreamOO";
  const streamoo_font_t *font = streamoo_ui_font();
  size_t width = streamoo_bitmap_text_width(font, name, sizeof name - 1);
  if (width > 89 || !streamoo_bitmap_text_line(
                        &small, (streamoo_rect_t){0, 0, 128, 16}, font, name,
                        sizeof name - 1, STREAMOO_TEXT_LEFT, 0, 0, 0, 15))
    return false;
  for (unsigned y = 0; y < 16; ++y)
    for (unsigned x = 0; x < width; ++x) {
      uint8_t packed = frame[y * 64 + x / 2],
              gray = x & 1 ? packed & 15 : packed >> 4;
      streamoo_canvas_fill(
          &canvas, (streamoo_rect_t){78 + (int)x * 2, 16 + (int)y * 2, 2, 2},
          gray);
    }
  memset(frame, 0, 1024);
  for (int y = 16; y <= 44; ++y)
    for (int x = 16; x <= 62; ++x) {
      uint8_t a = ring(x, y, 30), b = ring(x, y, 48);
      streamoo_canvas_pixel(&canvas, x, y, a > b ? a : b);
    }
  streamoo_canvas_line(&canvas, 18, 49, 27, 49, 5);
  streamoo_canvas_line(&canvas, 27, 49, 31, 45, 9);
  streamoo_canvas_line(&canvas, 31, 45, 36, 53, 12);
  streamoo_canvas_line(&canvas, 36, 53, 41, 46, 9);
  streamoo_canvas_line(&canvas, 41, 46, 45, 49, 5);
  streamoo_canvas_line(&canvas, 45, 49, 60, 49, 5);
  static const uint8_t label[] = "INTERNET RADIO";
  return streamoo_text_line(&canvas,
                            (streamoo_rect_t){78, 52, (int16_t)(width * 2), 8},
                            label, sizeof label - 1, STREAMOO_TEXT_CENTER, 0, 0,
                            0, 6, streamoo_font5x8, NULL);
}
