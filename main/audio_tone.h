#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define STREAMOO_TONE_RATE 48000u
#define STREAMOO_TONE_FRAMES (STREAMOO_TONE_RATE*10u)
#define STREAMOO_TONE_BLOCK_MAX 240u
#define STREAMOO_TONE_PEAK 1036
#define STREAMOO_TONE_RAMP 2400u
/* Diagnostic only: 1s zero, 2s L1k, 1s zero, 2s R2k, 1s zero,
 * 2s both1k, 1s zero. 50ms ramps. ~-30dBFS peak; no volume control. */
typedef struct { uint32_t position; } streamoo_audio_tone_t;
/* Bounded integer fill, 32-bit interleaved L/R. Saturates at end to silence.
 * No allocation/IO/libm. Reset position only while output disabled. */
bool streamoo_audio_tone_fill(streamoo_audio_tone_t *tone, int32_t *lr, size_t frames);
