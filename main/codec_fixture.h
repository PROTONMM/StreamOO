#pragma once
#include "esp_err.h"
#include "streamoo_decode_stream.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* One task owns decoder/cursor; separate reader owns compressed-file offset.
 * Release joins the reader before freeing borrowed storage. Never ISR. Pull decodes
 * with one packet in internal RAM, then converts into the internal PCM FIFO. */
esp_err_t streamoo_codec_fixture_prepare(void);
streamoo_stream_result_t streamoo_codec_fixture_read(int32_t *lr, size_t frames);

void streamoo_codec_fixture_release(void);
