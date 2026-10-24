#pragma once

#include "esp_err.h"

/* Validate installed memory and start low-priority health telemetry.
 * Called once, after board validation and before peripheral initialization. */
esp_err_t streamoo_platform_start(void);
