#pragma once
#include "streamoo_clock.h"
#include "esp_err.h"
/* Task startup once, no erase on NVS errors. Default Europe/Warsaw.
 * get is cached/short critical section, never flash I/O. set is serialized,
 * task-only NVS commit; cached zone changes ONLY after successful commit. */
esp_err_t streamoo_clock_settings_start(void);
streamoo_clock_zone_t streamoo_clock_settings_get(void);
esp_err_t streamoo_clock_settings_set(const char *iana_name);
