#pragma once
#include "esp_err.h"
#include "streamoo_config_model.h"
typedef struct {
  bool (*authorize)(const uint8_t *, size_t);
  bool (*wifi_load)(streamoo_wifi_profiles_t *);
  esp_err_t (*wifi_submit)(const streamoo_wifi_profiles_t *);
  esp_err_t (*playlist)(uint8_t *, size_t, bool, streamoo_playlist_report_t *);
  bool (*playlist_info)(streamoo_playlist_report_t *);
} streamoo_portal_actions_t;
/* Owner configures once before start; callbacks remain valid until stopped. */
esp_err_t streamoo_portal_configure(const streamoo_portal_actions_t *);
