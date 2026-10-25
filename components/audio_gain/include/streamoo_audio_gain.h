#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define STREAMOO_GAIN_UNITY 65536u
#define STREAMOO_GAIN_RAMP_FRAMES 2400u
#define STREAMOO_GAIN_BLOCK_MAX 240u
/* 48kHz / fixed 50ms linear amplitude ramp, stereo shares one gain per frame.
 * Single ISR owner; initialized while output disabled. No allocation or I/O. */
typedef struct {
    int32_t current, target, step, remainder, error;
    uint32_t remaining;
} streamoo_audio_gain_t;
void streamoo_audio_gain_init(streamoo_audio_gain_t *gain);
/* 0=mute, 100=unity, 1..100 proposed -59.4..0dB curve, table Q16. */
bool streamoo_audio_gain_volume(uint8_t volume, uint32_t *out);
bool streamoo_audio_gain_target(streamoo_audio_gain_t *gain, uint32_t target);
bool streamoo_audio_gain_process(streamoo_audio_gain_t *gain, int32_t *lr, size_t frames);
