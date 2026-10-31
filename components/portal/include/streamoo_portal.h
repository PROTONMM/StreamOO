#pragma once
#include "esp_err.h"
#include "streamoo_portal_status.h"
/* Server owner calls start/stop from a task, never a request handler or ISR.
 * Callback/context remain valid until successful stop. Getter must be bounded. */
typedef bool (*streamoo_portal_status_fn)(void *,streamoo_portal_status_t *);
esp_err_t streamoo_portal_start(streamoo_portal_status_fn,void *);
esp_err_t streamoo_portal_stop(void);
