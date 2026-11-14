#include <assert.h>
#include <string.h>
#include "ui_list_probe.h"

int main(void)
{
    uint8_t guarded[8194];
    memset(guarded, 0xa5, sizeof(guarded));
    streamoo_ui_list_probe_t probe;
    assert(streamoo_ui_list_probe_init(&probe));
    for (uint64_t time = 0; time <= 8000; time += 10) {
        assert(streamoo_ui_list_probe_draw(&probe, guarded + 1, 8192, time));
        assert(guarded[0] == 0xa5 && guarded[8193] == 0xa5);
        assert(probe.ui.playing_id == 1002);
    }
    assert(probe.requested_id == 1003);
    const int32_t deltas[] = {INT32_MAX, INT32_MIN, 17, -17, 0};
    for (unsigned i = 0; i < 1000; ++i) {
        const uint64_t time = 10000 + i * 10;
        assert(streamoo_ui_move(&probe.ui, deltas[i % 5], time));
        assert(probe.ui.selected < probe.ui.count);
        const int64_t position = streamoo_ui_position_q16(&probe.ui, time + 5);
        assert(position >= 0 && position <= 4 * 65536);
    }
    assert(streamoo_ui_set_catalog(&probe.ui, NULL, 0, 2));
    assert(streamoo_ui_open_stations(&probe.ui, 30000));
    assert(!streamoo_ui_confirm(&probe.ui));
    streamoo_canvas_t canvas;
    assert(streamoo_canvas_init(&canvas, guarded + 1, 8192, 256, 64, 128));
    assert(streamoo_ui_draw_list(&probe.ui, &canvas, (streamoo_rect_t){0, 8, 256, 56}, 30000));
    assert(guarded[0] == 0xa5 && guarded[8193] == 0xa5);
    return 0;
}
