#pragma once
#include "esp_err.h"
#include "streamoo_pcm_adapter.h"
typedef struct streamoo_mp3_decoder streamoo_mp3_decoder_t;
typedef struct {
    size_t consumed, produced, needed;
    int32_t codec_status;
    streamoo_pcm_format_t format;
} streamoo_mp3_result_t;
/* One decoder owner task. First open registers MP3 only; no default registry.
 * Process accepts arbitrary fragments; caller preserves unconsumed bytes.
 * Allocation/parser/codec calls are forbidden from ISR/audio sample callback.
 * No host binary implementation: vendor archives are for Espressif hardware. */
esp_err_t streamoo_mp3_open(streamoo_mp3_decoder_t **out);
esp_err_t streamoo_mp3_process(streamoo_mp3_decoder_t *decoder, uint8_t *input,
    size_t bytes, bool eos, uint8_t *pcm, size_t capacity, streamoo_mp3_result_t *result);
void streamoo_mp3_close(streamoo_mp3_decoder_t *decoder);
