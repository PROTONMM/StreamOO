#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct { uint32_t rate; uint8_t channels, bits; } streamoo_pcm_format_t;
/* Task-only converter. No resampling: accepts 48kHz, mono/stereo signed16 LE.
 * Duplicates mono; maps exact values to signed32 with multiplication by65536.
 * Up to240 frames, exact input size; invalid input leaves output untouched. */
bool streamoo_pcm16le_convert(streamoo_pcm_format_t format, const uint8_t *data,
                               size_t bytes, int32_t *lr, size_t frames);

/* Borrowed immutable PCM bytes, one task owns cursor. Diagnostic loop source.
 * Input/output must not overlap. Init rejects empty/partial/unsupported data. */
typedef struct {
    streamoo_pcm_format_t format;
    const uint8_t *data;
    size_t bytes, cursor;
} streamoo_pcm_clip_t;
bool streamoo_pcm_clip_init(streamoo_pcm_clip_t *clip, streamoo_pcm_format_t format,
                             const uint8_t *data, size_t bytes);
bool streamoo_pcm_clip_read(streamoo_pcm_clip_t *clip, int32_t *lr, size_t frames);
