#include "ssd1322_protocol.h"
#include <string.h>

static int command(ssd1322_protocol_t *d, uint8_t cmd,
                   const uint8_t *parameters, size_t length)
{
    int error = d->write(d->context, false, &cmd, 1);
    if (!error && length) {
        error = d->write(d->context, true, parameters, length);
    }
    return error;
}

int ssd1322_protocol_init(ssd1322_protocol_t *d, ssd1322_write_fn write,
                          void *context, const ssd1322_profile_t *p)
{
    if (!d || !write || !p || p->column_offset > 56 ||
        p->master_current > 15 || (p->remap & ~0x16)) {
        return -1;
    }
    *d = (ssd1322_protocol_t){.write = write, .context = context, .profile = *p};
    /* Independently specified from SSD1322 rev.1.2 command table.
     * Scan/phase timing uses the 256x64 panel reference (ADR0087), rather
     * than controller reset timing. Voltages/current remain conservative.
     * B[4]=1 enables dual COM for 64 rows; reserved B[0] must be one. */
    const struct { uint8_t cmd, count, data[2]; } setup[] = {
        {0xfd, 1, {0x12}}, {0xae, 0, {0}},
        {0xa0, 2, {p->remap, 0x11}}, {0xca, 1, {63}},
        {0xa1, 1, {0}}, {0xa2, 1, {0}}, {0xa9, 0, {0}},
        {0xab, 1, {1}}, {0xb3, 1, {0x91}}, {0xb1, 1, {0xe2}},
        {0xb4, 2, {0xa0, 0xb5}}, {0xb6, 1, {8}},
        {0xbb, 1, {0x17}}, {0xbe, 1, {4}},
        {0xc1, 1, {p->contrast}}, {0xc7, 1, {p->master_current}},
        {0xd1, 2, {0xa2, 0x20}}, {0xb9, 0, {0}}, {0xa6, 0, {0}},
    };
    for (size_t i = 0; i < sizeof(setup) / sizeof(setup[0]); ++i) {
        const int error = command(d, setup[i].cmd, setup[i].data, setup[i].count);
        if (error) {
            return error;
        }
    }
    d->ready = true;
    return 0;
}

static int present_rows(ssd1322_protocol_t *d, const uint8_t *frame,
                        unsigned first, unsigned count)
{
    const uint8_t columns[] = {d->profile.column_offset, d->profile.column_offset + 63};
    const uint8_t rows[] = {first, first + count - 1};
    int error = command(d, 0x15, columns, sizeof(columns));
    if (!error) error = command(d, 0x75, rows, sizeof(rows));
    if (!error) error = command(d, 0x5c, NULL, 0);
    if (!error) error = d->write(d->context, true, frame + first * (SSD1322_WIDTH / 2),
                                count * (SSD1322_WIDTH / 2));
    return error;
}

int ssd1322_protocol_present(ssd1322_protocol_t *d,
                             const uint8_t *frame, size_t length)
{
    if (!d || !d->ready || !frame || length != SSD1322_FRAME_BYTES) return -1;
    return present_rows(d, frame, 0, SSD1322_HEIGHT);
}

int ssd1322_protocol_present_changed(ssd1322_protocol_t *d,
    ssd1322_frame_cache_t *cache, const uint8_t *frame, size_t length,
    ssd1322_update_t *update)
{
    if (update) *update = (ssd1322_update_t){0};
    if (!d || !d->ready || !cache || !cache->pixels || !frame ||
        length != SSD1322_FRAME_BYTES) return -1;
    const size_t stride = SSD1322_WIDTH / 2;
    unsigned first = 0, end = SSD1322_HEIGHT;
    if (cache->valid) {
        while (first < end && !memcmp(frame + first * stride, cache->pixels + first * stride, stride)) ++first;
        if (first == end) return 0;
        while (end > first + 1 && !memcmp(frame + (end - 1) * stride, cache->pixels + (end - 1) * stride, stride)) --end;
    }
    cache->valid = false;
    const int error = present_rows(d, frame, first, end - first);
    if (error) return error;
    const size_t bytes = (end - first) * stride;
    memcpy(cache->pixels + first * stride, frame + first * stride, bytes);
    cache->valid = true;
    if (update) *update = (ssd1322_update_t){bytes, first, end - first};
    return 0;
}

int ssd1322_protocol_enable(ssd1322_protocol_t *d, bool enabled)
{
    if (!d || !d->ready) return -1;
    return command(d, enabled ? 0xaf : 0xae, NULL, 0);
}

int ssd1322_test_pattern(uint8_t *frame, size_t length)
{
    if (!frame || length != SSD1322_FRAME_BYTES) return -1;
    for (unsigned y = 0; y < SSD1322_HEIGHT; ++y) {
        for (unsigned x = 0; x < SSD1322_WIDTH; x += 2) {
            uint8_t pixels[2];
            for (unsigned i = 0; i < 2; ++i) {
                const unsigned px = x + i;
                uint8_t gray = px / 16;
                if (y == 0) gray = px % 16;
                else if (y == 63 || px == 0 || px == 255) gray = 15;
                else if (y >= 2 && y < 6 && px >= 2 && px < 6) gray = 15;
                else if (y >= 56 && y < 62 && px >= 246 && px < 254) gray = 4;
                pixels[i] = gray;
            }
            frame[y * (SSD1322_WIDTH / 2) + x / 2] = (pixels[0] << 4) | pixels[1];
        }
    }
    return 0;
}
