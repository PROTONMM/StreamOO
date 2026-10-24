#pragma once
#include "streamoo_ui.h"

typedef struct { streamoo_ui_t ui; } streamoo_home_probe_t;
bool streamoo_home_probe_init(streamoo_home_probe_t *probe);
/* Synthetic full-HOME snapshots, no clock/network/player services attached.
 * Monotonic elapsed; reinitialize before restarting this diagnostic. */
bool streamoo_home_probe_draw(streamoo_home_probe_t *probe, uint8_t *frame,
                               size_t bytes, uint64_t elapsed_ms);
