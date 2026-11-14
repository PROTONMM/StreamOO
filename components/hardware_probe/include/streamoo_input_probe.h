#pragma once

#include "esp_err.h"

/* Diagnostic IRQ capture and gesture queue. Start after board/platform checks. */
esp_err_t streamoo_input_probe_start(void);

#include "streamoo_input.h"
/* Single consumer, nonblocking. First call enables forwarding; events before
 * subscription are only logged. Full output queue drops and logs commands. */
bool streamoo_input_probe_receive(streamoo_input_event_t *event);
