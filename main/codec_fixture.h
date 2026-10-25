#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* One task owns decoder/cursor; not callable from ISR. Pull decodes on demand
 * with one packet in internal RAM, then converts into the internal PCM FIFO. */
esp_err_t streamoo_codec_fixture_prepare(void);
bool streamoo_codec_fixture_read(int32_t *lr, size_t frames);

void streamoo_codec_fixture_release(void);
