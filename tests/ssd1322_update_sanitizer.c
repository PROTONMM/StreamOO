#include "ssd1322_protocol.h"
#include <assert.h>
#include <string.h>

typedef struct {
    uint8_t ram[SSD1322_FRAME_BYTES], command, first, last;
    unsigned calls, fail_at;
} transport_t;

static int write_spi(void *context, bool data, const uint8_t *bytes, size_t length)
{
    transport_t *t = context;
    bool failed = ++t->calls == t->fail_at;
    if (!data) {
        assert(length == 1);
        t->command = bytes[0];
    } else if (t->command == 0x75) {
        assert(length == 2 && bytes[0] <= bytes[1] && bytes[1] < 64);
        t->first = bytes[0]; t->last = bytes[1];
    } else if (t->command == 0x5c) {
        assert(length == (size_t)(t->last - t->first + 1) * 128);
        /* Simulate an error after a partial payload has reached display RAM. */
        memcpy(t->ram + t->first * 128, bytes, failed ? length / 2 : length);
    }
    return failed ? 77 : 0;
}

int main(void)
{
    uint8_t source[SSD1322_FRAME_BYTES + 2], shadow[sizeof source];
    memset(source, 0xa5, sizeof source); memset(shadow, 0xa5, sizeof shadow);
    uint8_t *frame = source + 1;
    memset(frame, 0, SSD1322_FRAME_BYTES);
    transport_t transport = {0};
    memset(transport.ram, 0x55, sizeof transport.ram);
    ssd1322_protocol_t display;
    ssd1322_profile_t profile = {28, 0x14, 0x7f, 3};
    assert(ssd1322_protocol_init(&display, write_spi, &transport, &profile) == 0);
    ssd1322_frame_cache_t cache = {.pixels = shadow + 1};
    ssd1322_update_t update;
    unsigned random = 1237;
    for (unsigned i = 0; i < 2000; ++i) {
        if (i % 5) {
            random = random * 1664525u + 1013904223u;
            frame[random % SSD1322_FRAME_BYTES] ^= 0xff;
        }
        transport.calls = 0;
        transport.fail_at = i % 17 == 0 ? 6 : i % 19 == 0 ? 3 : 0;
        int error = ssd1322_protocol_present_changed(&display, &cache, frame,
                                                     SSD1322_FRAME_BYTES, &update);
        if (error) {
            assert(error == 77 && !cache.valid && update.bytes == 0);
            transport.fail_at = transport.calls = 0;
            assert(ssd1322_protocol_present_changed(&display, &cache, frame,
                                                    SSD1322_FRAME_BYTES, &update) == 0);
            assert(update.bytes == SSD1322_FRAME_BYTES);
        }
        assert(cache.valid);
        assert(memcmp(transport.ram, frame, SSD1322_FRAME_BYTES) == 0);
        assert(memcmp(cache.pixels, frame, SSD1322_FRAME_BYTES) == 0);
        transport.fail_at = transport.calls = 0;
        assert(ssd1322_protocol_present_changed(&display, &cache, frame,
                                                SSD1322_FRAME_BYTES, &update) == 0);
        assert(transport.calls == 0 && update.bytes == 0);
        assert(source[0] == 0xa5 && source[sizeof source - 1] == 0xa5);
        assert(shadow[0] == 0xa5 && shadow[sizeof shadow - 1] == 0xa5);
    }
    return 0;
}
