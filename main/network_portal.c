#include "network_portal.h"
#include "streamoo_portal.h"
#include "audio_control_probe.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
static bool status(void *context,streamoo_portal_status_t *out)
{
    (void)context;
    streamoo_network_state_t network;
    if (!out || !streamoo_network_state(&network)) return false;
    *out=(streamoo_portal_status_t){.ap_ready=network.ap_ready,.clients=network.clients,
        .uptime_ms=(uint64_t)(esp_timer_get_time()/1000)};
    memcpy(out->ap_ssid,network.ap_ssid,sizeof out->ap_ssid);
    streamoo_audio_control_state_t audio={0};
    if (streamoo_audio_control_state(&audio)) {
        out->audio_available=audio.ready;
        out->running=audio.model.running;
        out->buffering=audio.buffering;
        out->volume=audio.model.volume;
    }
    return true;
}
static void ready(void *context)
{
    (void)context;
    esp_err_t error=streamoo_portal_start(status,NULL);
    if (error!=ESP_OK) ESP_LOGE("portal","Portal unavailable: %s; AP/audio remain independent",esp_err_to_name(error));
}
static void stopped(void *context)
{
    (void)context;
    streamoo_portal_stop();
}
const streamoo_network_hooks_t streamoo_app_network_hooks={.ready=ready,.stopped=stopped};
