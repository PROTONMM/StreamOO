#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#define STREAMOO_AUDIO_RATE 48000
#define STREAMOO_AUDIO_DMA_FRAMES 240
#define STREAMOO_AUDIO_DMA_BUFFERS 4
/* Interleaved signed 32-bit L/R, 48kHz, 64 BCLK per frame, Philips I2S.
 * Called from ISR on a reusable DMA buffer. Function must be IRAM and context
 * internal SRAM. Fill all samples; no allocation, flash, blocking I/O, logs,
 * floating-point or FFT. Stop/start and resource allocation are startup/task only. */
typedef void (*streamoo_audio_fill_fn)(void *context, int32_t *lr, size_t frames);
typedef struct streamoo_audio_output streamoo_audio_output_t;
typedef struct { int dout, bclk, ws; } streamoo_audio_pins_t;
typedef struct { uint32_t buffers, max_fill_us, invalid_buffers; } streamoo_audio_stats_t;
esp_err_t streamoo_audio_output_open(streamoo_audio_pins_t pins, streamoo_audio_fill_fn fill,
                                      void *context, streamoo_audio_output_t **out);
bool streamoo_audio_output_stats(streamoo_audio_output_t *output, streamoo_audio_stats_t *out);
/* Single task owner. Disables channel before releasing ISR context. */
esp_err_t streamoo_audio_output_close(streamoo_audio_output_t *output);
