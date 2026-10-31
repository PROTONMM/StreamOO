#include "streamoo_network.h"
#include "sdkconfig.h"
#include "network_secret.h"
#if defined(CONFIG_STREAMOO_NETWORK_AP_TEST) && STREAMOO_NETWORK_SECRET_AVAILABLE
#include "streamoo_network_policy.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
static bool started;
static streamoo_network_hooks_t hooks;
static streamoo_network_state_t state;
static portMUX_TYPE state_lock=portMUX_INITIALIZER_UNLOCKED;
bool streamoo_network_state(streamoo_network_state_t *out)
{
    if (!out) return false;
    portENTER_CRITICAL(&state_lock); *out=state; portEXIT_CRITICAL(&state_lock);
    return true;
}
bool streamoo_network_ap_display(streamoo_network_ap_display_t *out)
{
    if (!out) return false;
    *out=(streamoo_network_ap_display_t){0};
    streamoo_network_state_t snapshot;
    if (!streamoo_network_state(&snapshot) || !snapshot.ap_ready) return false;
    memcpy(out->ssid,snapshot.ap_ssid,sizeof out->ssid);
    memcpy(out->password,streamoo_ap_password,sizeof streamoo_ap_password);
    out->password_bytes=sizeof streamoo_ap_password-1;
    return true;
}
static void network_task(void *context)
{
    (void)context;
    esp_netif_t *netif=NULL;
    bool initialized=false;
    uint8_t mac[6]; char ssid[16];
    esp_err_t error=nvs_flash_init(); /* No erase, including version/full errors. */
    if (error==ESP_OK) error=esp_netif_init();
    if (error==ESP_OK) error=esp_event_loop_create_default();
    if (error==ESP_OK) {
        netif=esp_netif_create_default_wifi_ap();
        if (!netif) error=ESP_ERR_NO_MEM;
    }
    wifi_init_config_t init=WIFI_INIT_CONFIG_DEFAULT();
    if (error==ESP_OK) { error=esp_wifi_init(&init); initialized=error==ESP_OK; }
    if (error==ESP_OK) error=esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (error==ESP_OK) error=esp_read_mac(mac,ESP_MAC_WIFI_SOFTAP);
    if (error==ESP_OK && !streamoo_network_ap_name(mac,ssid)) error=ESP_FAIL;
    wifi_config_t config={.ap={.ssid_len=15,.channel=1,.authmode=WIFI_AUTH_WPA2_PSK,
        .max_connection=4,.pmf_cfg={.required=false}}};
    if (error==ESP_OK) {
        memcpy(config.ap.ssid,ssid,15);
        memcpy(config.ap.password,streamoo_ap_password,sizeof streamoo_ap_password);
        error=esp_wifi_set_mode(WIFI_MODE_AP);
    }
    if (error==ESP_OK) error=esp_wifi_set_config(WIFI_IF_AP,&config);
    if (error==ESP_OK) {
        error=esp_netif_dhcps_stop(netif);
        if (error==ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) error=ESP_OK;
    }
    esp_netif_ip_info_t ip={.ip.addr=ESP_IP4TOADDR(192,168,4,1),
        .gw.addr=ESP_IP4TOADDR(192,168,4,1),.netmask.addr=ESP_IP4TOADDR(255,255,255,0)};
    if (error==ESP_OK) error=esp_netif_set_ip_info(netif,&ip);
    if (error==ESP_OK) error=esp_netif_dhcps_start(netif);
    if (error==ESP_OK) error=esp_wifi_start();
    if (error==ESP_OK) {
        ESP_LOGI("network","AP ready: SSID=%s WPA2; IPv4=192.168.4.1; no STA",ssid);
        portENTER_CRITICAL(&state_lock);
        memcpy(state.ap_ssid,ssid,sizeof state.ap_ssid); state.ap_ready=true;
        portEXIT_CRITICAL(&state_lock);
        if (hooks.ready) hooks.ready(hooks.context);
        for (;;) {
            wifi_sta_list_t clients={0};
            error=esp_wifi_ap_get_sta_list(&clients);
            if (error!=ESP_OK) break;
            portENTER_CRITICAL(&state_lock); state.clients=clients.num; portEXIT_CRITICAL(&state_lock);
            ESP_LOGI("network","AP clients=%d stack_hwm=%u",clients.num,
                (unsigned)uxTaskGetStackHighWaterMark(NULL));
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }
    ESP_LOGE("network","AP test failed: %s; no erase/fallback",esp_err_to_name(error));
    portENTER_CRITICAL(&state_lock); state.ap_ready=false; portEXIT_CRITICAL(&state_lock);
    if (hooks.stopped) hooks.stopped(hooks.context);
    if (initialized) { esp_wifi_stop(); esp_wifi_deinit(); }
    if (netif) esp_netif_destroy_default_wifi(netif);
    vTaskDelete(NULL);
}
esp_err_t streamoo_network_start(const streamoo_network_hooks_t *observer)
{
    if (started) return ESP_ERR_INVALID_STATE;
    if (!streamoo_network_password_valid(streamoo_ap_password,sizeof streamoo_ap_password-1)) return ESP_ERR_INVALID_ARG;
    hooks=observer ? *observer : (streamoo_network_hooks_t){0};
    if (xTaskCreate(network_task,"network",6144,NULL,3,NULL)!=pdPASS) return ESP_ERR_NO_MEM;
    started=true; return ESP_OK;
}
#else
bool streamoo_network_ap_display(streamoo_network_ap_display_t *out)
{
    if (out) *out=(streamoo_network_ap_display_t){0};
    return false;
}
esp_err_t streamoo_network_start(const streamoo_network_hooks_t *observer) { (void)observer; return ESP_ERR_NOT_FOUND; }
bool streamoo_network_state(streamoo_network_state_t *out)
{
    if (out) *out=(streamoo_network_state_t){0};
    return false;
}
#endif
