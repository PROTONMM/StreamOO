#pragma once

#include "esp_err.h"

/* Validate the complete menuconfig map before any peripheral initialization. */
esp_err_t streamoo_board_validate(void);
