#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Local OLED only. Clears full 256x64 frame; wraps password after42 characters
 * without truncating any of its63 bytes. Never logs/persists credentials. */
bool streamoo_ui_ap_draw(uint8_t *,size_t,const uint8_t *,size_t,const uint8_t *,size_t);
