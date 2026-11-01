#pragma once
#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif
/* Compile-time selection is shared by startup and periodic diagnostics. */
static inline const char *streamoo_flac_backend_name(void) {
#if CONFIG_STREAMOO_FLAC_MICRO
    return "micro-flac-0.3.0";
#elif CONFIG_STREAMOO_FLAC_DR
    return "dr_flac-0.13.4";
#else
    return "espressif-2.6.2";
#endif
}
