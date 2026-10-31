#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Original 256x64/4bpp StreamOO mark, no allocation or external bitmap. */
bool streamoo_ui_boot_draw(uint8_t *frame, size_t bytes);
