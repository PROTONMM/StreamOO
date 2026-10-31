#pragma once
#include "streamoo_decoder.h"
#include "streamoo_byte_source.h"
#define STREAMOO_STREAM_CHUNK 257u
#define STREAMOO_STREAM_SCRATCH 8192u
/* Single task owner. Decoder callbacks may allocate/block; never call from ISR.
 * Compressed source callback, bounded input and one decoded packet, no whole PCM clip.
 * Pull only when the downstream FIFO has space. Reset at EOF is fixture looping,
 * not production STOP/reconnect policy. */
typedef esp_err_t (*streamoo_stream_decode_fn)(void *, uint8_t *, size_t, bool,
    uint8_t *, size_t, streamoo_decode_result_t *);
typedef esp_err_t (*streamoo_stream_reset_fn)(void *);
typedef struct {
    streamoo_byte_source_t source;
    size_t pending, input_cursor, pcm_bytes, pcm_cursor, assembled, requested;
    int32_t block[480];
    bool input_eof;
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
typedef enum { STREAMOO_STREAM_READY, STREAMOO_STREAM_WAIT, STREAMOO_STREAM_FAILED } streamoo_stream_result_t;
bool streamoo_decode_stream_init(streamoo_decode_stream_t *, streamoo_byte_source_t,
    void *, streamoo_stream_decode_fn, streamoo_stream_reset_fn, size_t, size_t);
/* WAIT leaves output untouched and retains a partial block. Retry same frame count. */
streamoo_stream_result_t streamoo_decode_stream_read(streamoo_decode_stream_t *, int32_t *, size_t);
