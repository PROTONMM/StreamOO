#pragma once
#include "streamoo_text.h"

/* ASCII U+0020..007E and 18 Polish letters. Column bytes, bit0 at top.
 * Bitmap modification license remains OPEN; see THIRD_PARTY_NOTICES.md.
 * Unsupported Unicode returns false, without changing the output. */
bool streamoo_font5x8(void *context, uint32_t codepoint, uint8_t columns[5]);
