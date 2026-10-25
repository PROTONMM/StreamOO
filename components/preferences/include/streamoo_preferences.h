#pragma once
#include "streamoo_preferences_policy.h"
#include "esp_err.h"

/* Startup only, before UI: initialize/read existing NVS, no automatic erase.
 * On missing key use 50/spectrum without creating a preference record.
 * On invalid/unknown blob fail and preserve it; caller can use defaults.
 * Runtime flash writes are owned exclusively by the storage task. */
esp_err_t streamoo_preferences_start(void);
bool streamoo_preferences_initial(streamoo_preferences_value_t *out);
/* Nonblocking latest snapshot mailbox; one UI producer. Values copied.
 * 5s debounce from last submitted CHANGE, not each frame. No durable ack. */
bool streamoo_preferences_submit(streamoo_preferences_value_t value);

/* Explicit observed change, even if latest value equals previous after
 * coalescing. Same single producer; restarts idle deadline at observation. */
bool streamoo_preferences_submit_changed(streamoo_preferences_value_t value);
