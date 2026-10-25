#pragma once
#include "streamoo_decoder.h"
#define STREAMOO_STREAM_CHUNK 257u
#define STREAMOO_STREAM_SCRATCH 8192u
/* Single task owner. Decoder callbacks may allocate/block; never call from ISR.
 * Borrowed compressed file, bounded input and one decoded packet, no whole PCM clip.
 * Pull only when the downstream FIFO has space. Reset at EOF is fixture looping,
 * not production STOP/reconnect policy. */
typedef esp_err_t (*streamoo_stream_decode_fn)(void *, uint8_t *, size_t, bool,
    uint8_t *, size_t, streamoo_decode_result_t *);
typedef esp_err_t (*streamoo_stream_reset_fn)(void *);
typedef struct {
    const uint8_t *file;
    size_t bytes, offset, pending, input_cursor, pcm_bytes, pcm_cursor;
    uint8_t input[STREAMOO_STREAM_CHUNK];
    _Alignas(int32_t) uint8_t pcm[STREAMOO_STREAM_SCRATCH];
    void *context;
    streamoo_stream_decode_fn decode;
    streamoo_stream_reset_fn reset;
    streamoo_pcm_format_t format;
    uint32_t cycle_frames, last_cycle_frames, cycles, calls;
    size_t min_frames, max_frames;
    bool failed;
} streamoo_decode_stream_t;
bool streamoo_decode_stream_init(streamoo_decode_stream_t *, const uint8_t *, size_t,
    void *, streamoo_stream_decode_fn, streamoo_stream_reset_fn, size_t, size_t);
bool streamoo_decode_stream_read(streamoo_decode_stream_t *, int32_t *, size_t);
