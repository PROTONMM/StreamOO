#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Diagnostic geometry only, no font, metadata or audio data. */
bool streamoo_canvas_smoke(uint8_t *frame, size_t bytes);
