#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
typedef struct {
  uint32_t list_timeout_s; /* 1..120 */
  bool list_autoselect, autoplay;
  char hostname[64]; /* Empty: streamoo-MAC, otherwise DNS label1..63. */
} streamoo_settings_t;
streamoo_settings_t streamoo_settings_get(void);
esp_err_t streamoo_settings_start(void);
esp_err_t streamoo_settings_set(const streamoo_settings_t *);
bool streamoo_settings_valid(const streamoo_settings_t *);
uint64_t streamoo_settings_last_station(void);
/* Nonblocking owner publication; a separate task persists after5s idle. */
void streamoo_settings_station_changed(uint64_t id);
