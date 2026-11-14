#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Original 256x64/4bpp StreamOO mark, no allocation or external bitmap. */
bool streamoo_ui_boot_draw(uint8_t *frame, size_t bytes);

/* One-shot startup only: AP wins, short settling time, bounded audio wait. */
static inline bool streamoo_ui_boot_wait(bool ap_active, bool connected,
                                        bool audio_pending, uint64_t connected_ms) {
    return !ap_active && (!connected || connected_ms < 500 ||
                         (audio_pending && connected_ms < 30000));
}
