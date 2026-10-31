#include "network_portal.h"
#include "streamoo_portal.h"
#include "streamoo_portal_config.h"
#include "streamoo_config_store.h"
#include "audio_control_probe.h"
#include "streamoo_player.h"
#include "esp_log.h"
#include "streamoo_clock_settings.h"
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
    memcpy(out->sta_ip,network.sta_ip,sizeof out->sta_ip);
    out->sta_connected=network.sta_connected;out->configuring=network.configuring;
    out->saved_profiles=network.saved_profiles;out->network_error=network.last_error;
    if(streamoo_network_boot_ap_enabled()) {
        out->network_source=true;
        streamoo_player_state_t player;
        if(streamoo_player_state(&player)){
            out->audio_available=player.ready&&network.sta_connected&&!network.ap_ready;
            out->running=player.running;out->buffering=player.phase!=STREAMOO_PLAYER_PLAYING;
            out->volume=player.volume;out->station_id=player.station_id;
        }
        return true;
    }
    streamoo_audio_control_state_t audio={0};
    if (streamoo_audio_control_state(&audio)) {
        out->audio_available=audio.ready;
        out->running=audio.model.running;
        out->buffering=audio.buffering;
        out->volume=audio.model.volume;
    }
    return true;
}
static bool player_mode(void){
    streamoo_network_state_t n={0};return streamoo_network_state(&n)&&n.sta_connected&&!n.ap_ready;
}
static esp_err_t station_select(uint64_t id,uint64_t generation){
    streamoo_ui_select_command_t command={id,generation};
    return player_mode()&&streamoo_player_select(NULL,&command) ? ESP_OK:ESP_ERR_INVALID_STATE;
}
static esp_err_t player_command(streamoo_portal_player_action_t action,uint8_t volume)
{
    streamoo_network_state_t network={0};streamoo_player_state_t player;
    if(!streamoo_network_state(&network)||!network.sta_connected||network.ap_ready||
       !streamoo_player_state(&player)||!player.ready ||
       (action==STREAMOO_PORTAL_PLAY&&!player.station_id))return ESP_ERR_INVALID_STATE;
    bool queued=action==STREAMOO_PORTAL_VOLUME ? streamoo_player_set_volume(volume):
        streamoo_player_set_playing(action==STREAMOO_PORTAL_PLAY);
    return queued ? ESP_OK:ESP_FAIL;
}
static const char *clock_zone(void){return streamoo_clock_zone_name(streamoo_clock_settings_get());}
static void ready(void *context)
{
    (void)context;
    const streamoo_portal_actions_t actions={streamoo_config_wifi_load,
        streamoo_network_submit_profiles,streamoo_config_playlist,streamoo_config_playlist_info,clock_zone,streamoo_clock_settings_set,player_command,player_mode,station_select,streamoo_config_catalog_replace};
    esp_err_t error=streamoo_portal_configure(&actions);
    if(error==ESP_OK) error=streamoo_portal_start(status,NULL);
    if (error!=ESP_OK) ESP_LOGE("portal","Portal unavailable: %s; AP/audio remain independent",esp_err_to_name(error));
}
static void stopped(void *context)
{
    (void)context;
    streamoo_portal_stop();
}
const streamoo_network_hooks_t streamoo_app_network_hooks={.ready=ready,.stopped=stopped};
