#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    void (*ready)(void *);
    void (*stopped)(void *);
    void *context;
} streamoo_network_hooks_t;
typedef struct { char ap_ssid[16]; uint8_t clients; bool ap_ready; } streamoo_network_state_t;
/* Task-only owner starts once; hooks run in its task, outside state lock.
 * Hook/context lifetime extends to shutdown. Missing key keeps radio off. */
/* Bootstrap policy; true reserves this boot for AP configuration (no player). */
bool streamoo_network_boot_ap_enabled(void);
esp_err_t streamoo_network_start(const streamoo_network_hooks_t *);
bool streamoo_network_state(streamoo_network_state_t *);
/* Trusted local OLED projection only; never serialize to HTTP or logs.
 * Returns credentials only while AP ready. Key remains fixed in firmware. */
typedef struct { char ssid[16]; unsigned char password[64]; unsigned password_bytes; } streamoo_network_ap_display_t;
bool streamoo_network_ap_display(streamoo_network_ap_display_t *);
