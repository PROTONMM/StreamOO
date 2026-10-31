#pragma once
#include "esp_err.h"
/* Starts a task-only secured AP experiment. No credentials: ESP_ERR_NOT_FOUND,
 * radio remains off. No portal/STA/NVS credential storage in this stage. */
esp_err_t streamoo_network_start(void);
