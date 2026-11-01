#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SSD1322_WIDTH 256
#define SSD1322_HEIGHT 64
#define SSD1322_FRAME_BYTES (SSD1322_WIDTH * SSD1322_HEIGHT / 2)

/* Callback returns zero on success; nonzero transport errors propagate intact.
 * A command byte is sent with data=false, its parameters with data=true. */
typedef int (*ssd1322_write_fn)(void *context, bool data,
                                const uint8_t *bytes, size_t length);
typedef struct {
    uint8_t column_offset;  /* Units of four pixels, range 0..56. */
    uint8_t remap;          /* A0 first parameter, horizontal increment required. */
    uint8_t contrast;       /* C1 current, not pixel gray level. */
    uint8_t master_current; /* C7, 0..15. */
} ssd1322_profile_t;

typedef struct {
    ssd1322_write_fn write;
    void *context;
    ssd1322_profile_t profile;
    bool ready;
} ssd1322_protocol_t;

/* Caller-owned, non-overlapping 8192-byte copy of the last accepted frame.
 * Invalidate after any external reset/change of display RAM. No allocations. */
typedef struct {
    uint8_t *pixels;
    bool valid;
} ssd1322_frame_cache_t;
typedef struct {
    size_t bytes;           /* Pixel payload only; zero when identical. */
    uint8_t first_row, row_count;
} ssd1322_update_t;

/* Invalid arguments return -1 before writing anything.
 * Init configures the controller but leaves the panel OFF. Hardware reset and
 * power stabilization belong to the transport/board. No software reset exists. */
int ssd1322_protocol_init(ssd1322_protocol_t *display, ssd1322_write_fn write,
                          void *context, const ssd1322_profile_t *profile);
int ssd1322_protocol_present(ssd1322_protocol_t *display,
                             const uint8_t *frame, size_t length);
/* A single full-width row window from first to last changed row. Identical
 * frames perform no writes. First frame/invalid cache sends all 64 rows.
 * Commit cache only on success; transport failures invalidate it for retry. */
int ssd1322_protocol_present_changed(ssd1322_protocol_t *display,
    ssd1322_frame_cache_t *cache, const uint8_t *frame, size_t length,
    ssd1322_update_t *update);
int ssd1322_protocol_enable(ssd1322_protocol_t *display, bool enabled);
/* Diagnostic frame, independent from any renderer. First row alternates
 * nibbles; other rows show 16 bars, border and asymmetric corner markers. */
int ssd1322_test_pattern(uint8_t *frame, size_t length);
