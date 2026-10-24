#include <assert.h>
#include <string.h>
#include "streamoo_bitmap_font.h"
#include "streamoo_font5x8.h"

int main(void)
{
    uint8_t guarded[8194];
    memset(guarded, 0xa5, sizeof(guarded));
    streamoo_canvas_t canvas;
    assert(streamoo_canvas_init(&canvas, guarded + 1, 8192, 256, 64, 128));
    const uint8_t *text = (const uint8_t *)"Zażółć gęślą jaźń ĄĆĘŁŃÓŚŹŻ";
    const streamoo_font_t *font = streamoo_ui_font();
    for (int x = -320; x < 320; x += 7) {
        for (int y = -16; y < 70; y += 7) {
            assert(streamoo_bitmap_text_line(&canvas, (streamoo_rect_t){x, y, 150, 16},
                                             font, text, strlen((const char *)text),
                                             STREAMOO_TEXT_CENTER, UINT64_MAX, 30, 1000, 15));
            assert(streamoo_text_line(&canvas, (streamoo_rect_t){x, y, 150, 8},
                                       text, strlen((const char *)text), STREAMOO_TEXT_LEFT,
                                       UINT64_MAX, 30, 1000, 15, streamoo_font5x8, NULL));
            assert(guarded[0] == 0xa5 && guarded[8193] == 0xa5);
        }
    }
    return 0;
}
