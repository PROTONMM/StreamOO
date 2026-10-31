#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
    char ap_ssid[16];
    uint8_t clients, volume;
    bool ap_ready, audio_available, running, buffering;
    uint64_t uptime_ms;
    char sta_ip[16];
    bool sta_connected,configuring;
    uint8_t saved_profiles;
    uint32_t network_error;
    bool network_source;
    uint64_t station_id;
} streamoo_portal_status_t;
/* Public read-only DTO. Rejects invalid SSID/values; never send a failed render. */
bool streamoo_portal_status_json(const streamoo_portal_status_t *,char *,size_t);
