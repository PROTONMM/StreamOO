#include <assert.h>
#include <string.h>
#include "home_probe.h"

int main(void)
{
    uint8_t guarded[8194];
    memset(guarded, 0xa5, sizeof(guarded));
    streamoo_home_probe_t probe;
    assert(streamoo_home_probe_init(&probe));
    for (uint64_t time = 0; time < 20000; time += 20) {
        assert(streamoo_home_probe_draw(&probe, guarded + 1, 8192, time));
        assert(guarded[0] == 0xa5 && guarded[8193] == 0xa5);
        if (time >= 12000 && time < 15000) {
            assert(!probe.ui.home.metadata_valid);
            assert(probe.ui.home.title.bytes == 0 && probe.ui.home.codec.bytes == 0);
            assert(!probe.ui.home.visualization_valid);
        }
        if (time >= 18000) {
            assert(probe.ui.playing_id == 0);
            assert(!probe.ui.home.clock_valid && !probe.ui.home.wifi_connected);
        }
    }
    return 0;
}
