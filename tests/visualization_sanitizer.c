#include <assert.h>
#include <string.h>
#include "streamoo_visualization.h"

int main(void)
{
    uint8_t guarded[8194];
    memset(guarded, 0xa5, sizeof(guarded));
    streamoo_canvas_t canvas;
    assert(streamoo_canvas_init(&canvas, guarded + 1, 8192, 256, 64, 128));
    const int16_t widths[] = {8, 9, 132, 133, 256};
    const int16_t heights[] = {8, 9, 32, 33, 64};
    for (unsigned w = 0; w < 5; ++w) {
        for (unsigned h = 0; h < 5; ++h) {
            const streamoo_rect_t bounds = {256 - widths[w], 64 - heights[h], widths[w], heights[h]};
            for (unsigned level = 0; level <= 32767; level += 1023) {
                streamoo_visualization_model_t model = {.band_count = 64};
                model.rms[0] = level;
                model.rms[1] = 32767 - level;
                model.peak[0] = model.peak[1] = 32767;
                for (unsigned i = 0; i < 64; ++i) model.bands[i] = (level + i * 400) % 32768;
                uint8_t before[sizeof(model)];
                memcpy(before, &model, sizeof(model));
                for (unsigned kind = 0; kind < 3; ++kind) {
                    assert(streamoo_visualization_draw(&canvas, bounds, kind, &model));
                    assert(memcmp(&model, before, sizeof(model)) == 0);
                    assert(guarded[0] == 0xa5 && guarded[8193] == 0xa5);
                }
            }
        }
    }
    return 0;
}
