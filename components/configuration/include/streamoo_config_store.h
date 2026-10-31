#pragma once
#include "esp_err.h"
#include "streamoo_config_model.h"
/* Task-only persistence. Call init once before exposing write routes. */
esp_err_t streamoo_config_store_init(void);
bool streamoo_config_wifi_load(streamoo_wifi_profiles_t *);
esp_err_t streamoo_config_wifi_save(const streamoo_wifi_profiles_t *);
/* Validates/normalizes an upload REQUEST COPY; commit atomically renames in
 * LittleFS. */
esp_err_t streamoo_config_playlist(uint8_t *, size_t, bool,
                                   streamoo_playlist_report_t *);
bool streamoo_config_playlist_info(streamoo_playlist_report_t *);
