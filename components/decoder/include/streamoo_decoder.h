#pragma once
#include "esp_err.h"
#include "streamoo_pcm_adapter.h"
/* AAC_LC is the legacy enum name for the complete ADTS AAC family. */
typedef enum { STREAMOO_CODEC_MP3, STREAMOO_CODEC_AAC_LC, STREAMOO_CODEC_FLAC, STREAMOO_CODEC_VORBIS, STREAMOO_CODEC_OPUS, STREAMOO_CODEC_COUNT } streamoo_codec_t;
typedef struct streamoo_decoder streamoo_decoder_t;
typedef struct {
    size_t consumed, produced, needed;
    int32_t codec_status;
    streamoo_pcm_format_t format;
} streamoo_decode_result_t;
/* One decoder owner task. First open registers only the requested codec/container; no default registry. AAC includes LC/HE/HEv2 in ADTS; Vorbis/Opus use Ogg.
 * Process accepts arbitrary fragments; caller preserves unconsumed bytes.
 * Allocation/parser/codec calls are forbidden from ISR/audio sample callback.
 * No host binary implementation: vendor archives are for Espressif hardware. */
esp_err_t streamoo_decoder_open(streamoo_codec_t codec, streamoo_decoder_t **out);
esp_err_t streamoo_decoder_process(streamoo_decoder_t *decoder, uint8_t *input,
    size_t bytes, bool eos, uint8_t *pcm, size_t capacity, streamoo_decode_result_t *result);
esp_err_t streamoo_decoder_reset(streamoo_decoder_t *decoder);
void streamoo_decoder_close(streamoo_decoder_t *decoder);
