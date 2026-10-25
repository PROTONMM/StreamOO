#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* One task owns preparation/cursor; not callable from ISR. Immutable PCM in
 * PSRAM after prepare, converted into internal PCM FIFO in producer task. */
esp_err_t streamoo_mp3_fixture_prepare(void);
bool streamoo_mp3_fixture_read(int32_t *lr, size_t frames);

void streamoo_mp3_fixture_release(void);
