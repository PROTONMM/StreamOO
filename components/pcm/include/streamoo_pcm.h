#pragma once
#include <stdatomic.h>
#include "streamoo_audio_gain.h"
/* Fixed stereo signed32/48kHz diagnostic pipeline. */
#define STREAMOO_PCM_FRAMES 240u
#define STREAMOO_PCM_WORDS (STREAMOO_PCM_FRAMES*2u)
#define STREAMOO_PCM_LOW_BLOCKS 10u
#define STREAMOO_PCM_START_BLOCKS 24u

typedef struct { int32_t samples[STREAMOO_PCM_WORDS]; uint32_t sequence; } streamoo_pcm_block_t;
/* Exactly one producer and one consumer. Storage/context in internal RAM for
 * ISR use. Sequence is caller-provided; optional output on pop detects gaps.
 * Power-of-two capacity 2..64. Initialize/reset only while quiescent.
 * Release/acquire counters; no allocation, waiting, mutex or overwrite. */
typedef struct {
    streamoo_pcm_block_t *storage;
    uint32_t capacity;
    atomic_uint read, write;
} streamoo_pcm_ring_t;
bool streamoo_pcm_ring_init(streamoo_pcm_ring_t *ring, streamoo_pcm_block_t *storage, uint32_t capacity);
bool streamoo_pcm_push(streamoo_pcm_ring_t *ring, const int32_t *samples, uint32_t sequence);
bool streamoo_pcm_pop(streamoo_pcm_ring_t *ring, int32_t *samples, uint32_t *sequence);
/* Exact for the owning producer/consumer; clamped approximate observer count. */
uint32_t streamoo_pcm_count(const streamoo_pcm_ring_t *ring);

typedef enum { STREAMOO_PCM_BUFFERING, STREAMOO_PCM_ACTIVE, STREAMOO_PCM_FADING } streamoo_pcm_mode_t;
typedef struct {
    uint32_t output_blocks, received, zero_blocks, underflows, hard_underflows, restarts, tap_drops;
    streamoo_pcm_mode_t mode;
    uint32_t queued;
} streamoo_pcm_stats_t;
typedef struct {
    streamoo_pcm_ring_t *source, *tap;
    streamoo_audio_gain_t gain;
    int32_t last[2];
    bool hard_seen;
    streamoo_pcm_stats_t stats;
} streamoo_pcm_pipeline_t;
/* Single consumer/ISR owner. tap may be NULL. Source capacity >=24.
 * Before-gain tap is drop-newest on full, including zero blocks in buffering.
 * 10-block reserve permits 50ms fade on real PCM; restart at >=24 blocks.
 * Unexpected hard empty holds last raw sample only while fading to zero. */
bool streamoo_pcm_pipeline_init(streamoo_pcm_pipeline_t *pipe, streamoo_pcm_ring_t *source, streamoo_pcm_ring_t *tap);
bool streamoo_pcm_fill(streamoo_pcm_pipeline_t *pipe, int32_t *lr, size_t frames, uint32_t target_gain);
