#pragma once

#include "esp_err.h"

/* Validate installed memory and start low-priority health telemetry.
 * Called once, after board validation and before peripheral initialization. */
esp_err_t streamoo_platform_start(void);

/* Boot-only crypto known-answer checks, before any audio/network task starts.
 * Enabled by the experimental early-init profile; otherwise a no-op. */
esp_err_t streamoo_platform_crypto_prepare(void);
