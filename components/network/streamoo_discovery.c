#include "streamoo_discovery.h"
#include "mdns.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static bool started;
static int64_t retry_at;

static void hostname_changed(const char *hostname, void *context)
{
    (void)context;
    ESP_LOGI("discovery", "Player address: http://%s.local/", hostname);
}

void streamoo_discovery_update(bool connected, const char *ap_name, int64_t now_us)
{
    if (!connected) {
        if (started) mdns_free();
        started = false;
        retry_at = 0;
        return;
    }
    if (started || now_us < retry_at) return;
    /* AP name StreamOO-XXXXXX comes from the board MAC, not user input. */
    if (!ap_name || strlen(ap_name) != 15 || strncmp(ap_name, "StreamOO-", 9)) return;
    char hostname[16] = "streamoo-";
    for (unsigned i = 9; i < 15; ++i) {
        char c = ap_name[i];
        if (c >= 'A' && c <= 'F') c += 'a' - 'A';
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return;
        hostname[i] = c;
    }
    retry_at = now_us + 30000000;
    esp_err_t error = mdns_init();
    bool initialized = error == ESP_OK;
    if (error == ESP_OK)
        error = mdns_register_hostname_changed_callback(hostname_changed, NULL);
    if (error == ESP_OK) error = mdns_hostname_set(hostname);
    if (error == ESP_OK) error = mdns_instance_name_set("StreamOO");
    if (error == ESP_OK) error = mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
    if (error == ESP_OK) {
        started = true;
        ESP_LOGI("discovery", "mDNS started: http://%s.local/", hostname);
    } else {
        if (initialized) mdns_free();
        ESP_LOGW("discovery", "mDNS unavailable (%s); IP access retained", esp_err_to_name(error));
    }
}
