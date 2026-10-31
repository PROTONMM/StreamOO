#include "streamoo_discovery.h"
#include "streamoo_network.h"
#include "network_secret.h"
#include "sdkconfig.h"
#if defined(CONFIG_STREAMOO_NETWORK_AP_TEST) &&                                \
    STREAMOO_NETWORK_SECRET_AVAILABLE
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "streamoo_config_store.h"
#include "streamoo_network_policy.h"
#include <stdio.h>
#include <string.h>
bool streamoo_network_boot_ap_enabled(void) {
  return streamoo_network_password_valid(streamoo_ap_password,
                                         sizeof streamoo_ap_password - 1);
}
static bool started;
static streamoo_network_hooks_t hooks;
static streamoo_network_state_t state;
static portMUX_TYPE state_lock = portMUX_INITIALIZER_UNLOCKED;
static QueueHandle_t changes;
static EventGroupHandle_t events;
static streamoo_wifi_profiles_t saved, candidate;
static bool sntp_started, sta_inflight;
#define GOT_IP 1u
#define DISCONNECTED 2u
bool streamoo_network_state(streamoo_network_state_t *out) {
  if (!out)
    return false;
  portENTER_CRITICAL(&state_lock);
  *out = state;
  if(!out->sta_connected)out->rssi_valid=false;
  portEXIT_CRITICAL(&state_lock);
  return true;
}
bool streamoo_network_ap_display(streamoo_network_ap_display_t *out) {
  if (!out)
    return false;
  *out = (streamoo_network_ap_display_t){0};
  streamoo_network_state_t snapshot;
  if (!streamoo_network_state(&snapshot) || !snapshot.ap_ready)
    return false;
  memcpy(out->ssid, snapshot.ap_ssid, sizeof out->ssid);
  memcpy(out->password, streamoo_ap_password, sizeof streamoo_ap_password);
  out->password_bytes = sizeof streamoo_ap_password - 1;
  return true;
}
esp_err_t
streamoo_network_submit_profiles(const streamoo_wifi_profiles_t *profiles) {
  if (!streamoo_wifi_profiles_valid(profiles))
    return ESP_ERR_INVALID_ARG;
  portENTER_CRITICAL(&state_lock);
  bool allowed = changes && !state.configuring;
  if (allowed) {
    state.configuring = true;
    state.last_error = 0;
  }
  portEXIT_CRITICAL(&state_lock);
  if (!allowed)
    return ESP_ERR_INVALID_STATE;
  if (xQueueSend(changes, profiles, 0) != pdTRUE) {
    portENTER_CRITICAL(&state_lock);
    state.configuring = false;
    portEXIT_CRITICAL(&state_lock);
    return ESP_ERR_TIMEOUT;
  }
  return ESP_OK;
}
static void event(void *context, esp_event_base_t base, int32_t id,
                  void *data) {
  (void)context;
  (void)data;
  if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    xEventGroupSetBits(events, GOT_IP);
  if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)
    xEventGroupSetBits(events, DISCONNECTED);
}
static void time_synchronized(struct timeval *tv) {
  bool valid=tv && (int64_t)tv->tv_sec>=1577836800 && (int64_t)tv->tv_sec<4102444800LL;
  portENTER_CRITICAL(&state_lock);state.time_synced=valid;portEXIT_CRITICAL(&state_lock);
}
static void disconnect_station(void) {
  xEventGroupClearBits(events, GOT_IP | DISCONNECTED);
  if (sta_inflight) {
    if (esp_wifi_disconnect() == ESP_OK)
      xEventGroupWaitBits(events, DISCONNECTED, pdTRUE, pdFALSE,
                          pdMS_TO_TICKS(500));
    sta_inflight = false;
  }
  xEventGroupClearBits(events, GOT_IP | DISCONNECTED);
}
static bool connect_profiles(const streamoo_wifi_profiles_t *profiles,
                             esp_netif_t *sta) {
  portENTER_CRITICAL(&state_lock);
  state.sta_connected = false;
  state.sta_ip[0] = 0;
  state.sta_ssid[0] = 0;
  portEXIT_CRITICAL(&state_lock);
  for (unsigned i = 0; i < profiles->count; ++i) {
    disconnect_station();
    wifi_config_t config = {0};
    const streamoo_wifi_profile_t *p = &profiles->profiles[i];
    memcpy(config.sta.ssid, p->ssid, strlen(p->ssid));
    memcpy(config.sta.password, p->password, strlen(p->password));
    config.sta.threshold.authmode =
        p->password[0] ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    config.sta.scan_method = WIFI_FAST_SCAN;
    config.sta.pmf_cfg.required = false;
    if (esp_wifi_set_config(WIFI_IF_STA, &config) != ESP_OK)
      continue;
    sta_inflight = true;
    if (esp_wifi_connect() != ESP_OK) {
      sta_inflight = false;
      continue;
    }
    ESP_LOGI("network", "STA attempt profile=%u/%u", i + 1, profiles->count);
    EventBits_t bits = xEventGroupWaitBits(
        events, GOT_IP | DISCONNECTED, pdTRUE, pdFALSE, pdMS_TO_TICKS(15000));
    if (bits & DISCONNECTED)
      sta_inflight = false;
    if (bits & GOT_IP) {
      wifi_ap_record_t connected = {0};
      esp_netif_ip_info_t ip = {0};
      if (esp_wifi_sta_get_ap_info(&connected) != ESP_OK ||
          strncmp((char *)connected.ssid, p->ssid, 32) ||
          esp_netif_get_ip_info(sta, &ip) != ESP_OK || !ip.ip.addr)
        continue;
      sta_inflight = true;
      char address[16] = {0};
      snprintf(address, sizeof address, IPSTR, IP2STR(&ip.ip));
      portENTER_CRITICAL(&state_lock);
      state.sta_connected = true;
      state.rssi_valid = connected.rssi>=-127;
      state.rssi_dbm = connected.rssi>0 ? 0 : connected.rssi;
      memcpy(state.sta_ip, address, sizeof state.sta_ip);
      memcpy(state.sta_ssid, p->ssid, sizeof state.sta_ssid);
      portEXIT_CRITICAL(&state_lock);
      if (!sntp_started) {
        esp_sntp_config_t time_config =
            ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
        time_config.wait_for_sync = false;
        time_config.sync_cb = time_synchronized;
        esp_err_t time_error = esp_netif_sntp_init(&time_config);
        sntp_started = time_error == ESP_OK;
        if (time_error != ESP_OK)
          ESP_LOGW("network", "SNTP unavailable: %s",
                   esp_err_to_name(time_error));
      }
      ESP_LOGI("network", "STA connected: IPv4=%s", address);
      return true;
    }
  }
  disconnect_station();
  portENTER_CRITICAL(&state_lock);
  state.sta_connected = false;
  state.sta_ip[0] = 0;
  state.sta_ssid[0] = 0;
  portEXIT_CRITICAL(&state_lock);
  return false;
}
static bool enable_ap(void) {
  streamoo_network_state_t snapshot;
  streamoo_network_state(&snapshot);
  if (snapshot.ap_ready)
    return true;
  if (esp_wifi_set_mode(WIFI_MODE_APSTA) != ESP_OK)
    return false;
  portENTER_CRITICAL(&state_lock);
  state.ap_ready = true;
  state.clients = 0;
  portEXIT_CRITICAL(&state_lock);
  ESP_LOGI("network", "AP configuration ready: IPv4=192.168.4.1");
  return true;
}
static bool ap_off(void) {
  if (esp_wifi_set_mode(WIFI_MODE_STA) == ESP_OK) {
    portENTER_CRITICAL(&state_lock);
    state.ap_ready = false;
    state.clients = 0;
    portEXIT_CRITICAL(&state_lock);
    ESP_LOGI("network",
             "AP disabled after STA IPv4; portal available on STA address");
    return true;
  }
  return false;
}
static void network_task(void *context) {
  (void)context;
  esp_netif_t *ap = NULL, *sta = NULL;
  bool initialized = false;
  esp_event_handler_instance_t wifi_handler = NULL, ip_handler = NULL;
  esp_err_t error = streamoo_config_store_init();
  if (error != ESP_OK)
    ESP_LOGW("network",
             "Station storage unavailable: %s; existing data retained",
             esp_err_to_name(error));
  if (!streamoo_config_wifi_load(&saved))
    ESP_LOGW("network", "Wi-Fi profiles unavailable; keeping AP for recovery");
  portENTER_CRITICAL(&state_lock);
  state.saved_profiles = saved.count;
  portEXIT_CRITICAL(&state_lock);
  error = esp_netif_init();
  if (error == ESP_OK)
    error = esp_event_loop_create_default();
  if (error == ESP_OK) {
    ap = esp_netif_create_default_wifi_ap();
    sta = esp_netif_create_default_wifi_sta();
    if (!ap || !sta)
      error = ESP_ERR_NO_MEM;
  }
  wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
  if (error == ESP_OK) {
    error = esp_wifi_init(&init);
    initialized = error == ESP_OK;
  }
  if (error == ESP_OK)
    error = esp_wifi_set_storage(WIFI_STORAGE_RAM);
  if (error == ESP_OK)
    error = esp_event_handler_instance_register(
        WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, event, NULL, &wifi_handler);
  if (error == ESP_OK)
    error = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                event, NULL, &ip_handler);
  uint8_t mac[6];
  char ssid[16];
  if (error == ESP_OK)
    error = esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
  if (error == ESP_OK && !streamoo_network_ap_name(mac, ssid))
    error = ESP_FAIL;
  wifi_config_t config = {.ap = {.ssid_len = 15,
                                 .channel = 1,
                                 .authmode = WIFI_AUTH_WPA2_PSK,
                                 .max_connection = 4,
                                 .pmf_cfg = {.required = false}}};
  if (error == ESP_OK) {
    memcpy(config.ap.ssid, ssid, 15);
    memcpy(config.ap.password, streamoo_ap_password,
           sizeof streamoo_ap_password);
    error = esp_wifi_set_mode(WIFI_MODE_APSTA);
  }
  if (error == ESP_OK)
    error = esp_wifi_set_config(WIFI_IF_AP, &config);
  if (error == ESP_OK) {
    error = esp_netif_dhcps_stop(ap);
    if (error == ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED)
      error = ESP_OK;
  }
  esp_netif_ip_info_t address = {.ip.addr = ESP_IP4TOADDR(192, 168, 4, 1),
                                 .gw.addr = ESP_IP4TOADDR(192, 168, 4, 1),
                                 .netmask.addr =
                                     ESP_IP4TOADDR(255, 255, 255, 0)};
  if (error == ESP_OK)
    error = esp_netif_set_ip_info(ap, &address);
  if (error == ESP_OK)
    error = esp_netif_dhcps_start(ap);
  if (error == ESP_OK)
    error = esp_wifi_set_mode(WIFI_MODE_STA);
  if (error == ESP_OK)
    error = esp_wifi_start();
  if (error == ESP_OK) {
    portENTER_CRITICAL(&state_lock);
    memcpy(state.ap_ssid, ssid, sizeof state.ap_ssid);
    state.ap_ready = false;
    portEXIT_CRITICAL(&state_lock);
    ESP_LOGI("network", "STA first: trying %u saved profiles before AP",
             saved.count);
    bool boot_connected = saved.count && connect_profiles(&saved, sta);
    if (!boot_connected && !enable_ap()) {
      error = ESP_FAIL;
      goto startup_failed;
    }
    if (hooks.ready)
      hooks.ready(hooks.context);
    int64_t retry_at = saved.count ? esp_timer_get_time() + 1000000 : INT64_MAX,
            last_log = 0, off_at = INT64_MAX, rssi_at = 0;
    unsigned retry_seconds = 1;
    for (;;) {
      bool update =
          xQueueReceive(changes, &candidate, pdMS_TO_TICKS(100)) == pdTRUE;
      int64_t now = esp_timer_get_time();
      streamoo_network_state_t snapshot;
      streamoo_network_state(&snapshot);
      if (update) {
        off_at = INT64_MAX;
        esp_err_t saved_error = ESP_FAIL;
        /* HTTP 202 must leave before the current LAN connection is closed. */
        vTaskDelay(pdMS_TO_TICKS(750));
        bool connected = candidate.count && connect_profiles(&candidate, sta);
        if (!candidate.count) {
          disconnect_station();
          portENTER_CRITICAL(&state_lock);
          state.sta_connected = false;
          state.sta_ip[0] = 0;
          state.sta_ssid[0] = 0;
          portEXIT_CRITICAL(&state_lock);
          saved_error = streamoo_config_wifi_save(&candidate);
          if (saved_error == ESP_OK) {
            saved = candidate;
            retry_at = INT64_MAX;
          }
        } else if (connected) {
          saved_error = streamoo_config_wifi_save(&candidate);
          if (saved_error == ESP_OK) {
            saved = candidate;
            retry_seconds = 1;
          }
        }
        if (saved_error != ESP_OK) {
          /* A failed edit does not discard the last working list. Try it
           * before offering AP recovery on a previously STA-only device. */
          connected = saved.count && connect_profiles(&saved, sta);
          if (!saved.count) {
            disconnect_station();
            portENTER_CRITICAL(&state_lock);
            state.sta_connected = false;
            state.sta_ip[0] = 0;
            state.sta_ssid[0] = 0;
            portEXIT_CRITICAL(&state_lock);
          }
        } else if (!candidate.count)
          connected = false;
        if (connected) {
          streamoo_network_state(&snapshot);
          off_at =
              snapshot.ap_ready ? esp_timer_get_time() + 5000000 : INT64_MAX;
        } else {
          if (!enable_ap())
            ESP_LOGE("network", "AP recovery startup failed");
          retry_at = saved.count ? esp_timer_get_time() + 1000000 : INT64_MAX;
        }
        portENTER_CRITICAL(&state_lock);
        state.configuring = false;
        state.saved_profiles = saved.count;
        state.last_error = saved_error == ESP_OK ? 0 : 1;
        portEXIT_CRITICAL(&state_lock);
        if (saved_error != ESP_OK) {
          ESP_LOGW("network",
                   "Candidate profiles rejected; prior profiles retained");
          retry_at = saved.count ? esp_timer_get_time() + 2000000 : INT64_MAX;
        }
        memset(&candidate, 0, sizeof candidate);
      } else if (saved.count && !snapshot.sta_connected && now >= retry_at) {
        if (connect_profiles(&saved, sta)) {
          off_at = esp_timer_get_time() + 5000000;
          retry_seconds = 1;
        } else {
          if (!snapshot.ap_ready && !enable_ap())
            ESP_LOGE("network", "AP recovery startup failed");
          retry_at = esp_timer_get_time() + (int64_t)retry_seconds * 1000000;
          retry_seconds = retry_seconds < 8 ? retry_seconds * 2 : 15;
        }
      }
      streamoo_network_state(&snapshot);
      now = esp_timer_get_time();
      if (snapshot.sta_connected &&
          (xEventGroupGetBits(events) & DISCONNECTED)) {
        wifi_ap_record_t current;
        if (esp_wifi_sta_get_ap_info(&current) != ESP_OK) {
          sta_inflight = false;
          portENTER_CRITICAL(&state_lock);
          state.sta_connected = false;
          state.sta_ip[0] = 0;
          state.sta_ssid[0] = 0;
          portEXIT_CRITICAL(&state_lock);
          retry_at = now + 1000000;
          off_at = INT64_MAX;
        }
        xEventGroupClearBits(events, DISCONNECTED);
      }
      streamoo_network_state(&snapshot);
      if (snapshot.sta_connected && snapshot.ap_ready && now >= off_at) {
        off_at = ap_off() ? INT64_MAX : now + 1000000;
      }
      streamoo_network_state(&snapshot);
      streamoo_discovery_update(snapshot.sta_connected && !snapshot.ap_ready, snapshot.ap_ssid, now);
      if(snapshot.sta_connected && now>=rssi_at) {
        wifi_ap_record_t info={0};
        bool valid=esp_wifi_sta_get_ap_info(&info)==ESP_OK && info.rssi>=-127;
        portENTER_CRITICAL(&state_lock);
        state.rssi_valid=valid;
        state.rssi_dbm=valid && info.rssi<0 ? info.rssi : 0;
        portEXIT_CRITICAL(&state_lock);
        rssi_at=now+1000000;
      }
      if (now - last_log >= 10000000) {
        wifi_sta_list_t clients = {0};
        if (snapshot.ap_ready)
          esp_wifi_ap_get_sta_list(&clients);
        portENTER_CRITICAL(&state_lock);
        state.clients = clients.num;
        portEXIT_CRITICAL(&state_lock);
        ESP_LOGI("network", "AP clients=%d STA=%u saved=%u stack_hwm=%u",
                 clients.num, (unsigned)snapshot.sta_connected,
                 snapshot.saved_profiles,
                 (unsigned)uxTaskGetStackHighWaterMark(NULL));
        last_log = now;
      }
    }
  }
startup_failed:
  ESP_LOGE("network", "Network startup failed: %s; no erase/fallback",
           esp_err_to_name(error));
  portENTER_CRITICAL(&state_lock);
  state.ap_ready = false;
  portEXIT_CRITICAL(&state_lock);
  if (hooks.stopped)
    hooks.stopped(hooks.context);
  if (wifi_handler)
    esp_event_handler_instance_unregister(
        WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, wifi_handler);
  if (ip_handler)
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                          ip_handler);
  if (initialized) {
    esp_wifi_stop();
    esp_wifi_deinit();
  }
  if (ap)
    esp_netif_destroy_default_wifi(ap);
  if (sta)
    esp_netif_destroy_default_wifi(sta);
  vTaskDelete(NULL);
}
esp_err_t streamoo_network_start(const streamoo_network_hooks_t *observer) {
  if (started)
    return ESP_ERR_INVALID_STATE;
  if (!streamoo_network_boot_ap_enabled())
    return ESP_ERR_INVALID_ARG;
  changes = xQueueCreate(1, sizeof(streamoo_wifi_profiles_t));
  events = xEventGroupCreate();
  if (!changes || !events) {
    if (changes)
      vQueueDelete(changes);
    if (events)
      vEventGroupDelete(events);
    changes = NULL;
    events = NULL;
    return ESP_ERR_NO_MEM;
  }
  hooks = observer ? *observer : (streamoo_network_hooks_t){0};
  if (xTaskCreate(network_task, "network", 8192, NULL, 3, NULL) != pdPASS) {
    vQueueDelete(changes);
    vEventGroupDelete(events);
    changes = NULL;
    events = NULL;
    return ESP_ERR_NO_MEM;
  }
  started = true;
  return ESP_OK;
}
#else
bool streamoo_network_boot_ap_enabled(void) { return false; }
bool streamoo_network_ap_display(streamoo_network_ap_display_t *out) {
  if (out)
    *out = (streamoo_network_ap_display_t){0};
  return false;
}
esp_err_t streamoo_network_submit_profiles(const streamoo_wifi_profiles_t *p) {
  (void)p;
  return ESP_ERR_NOT_SUPPORTED;
}
esp_err_t streamoo_network_start(const streamoo_network_hooks_t *observer) {
  (void)observer;
  return ESP_ERR_NOT_FOUND;
}
bool streamoo_network_state(streamoo_network_state_t *out) {
  if (out)
    *out = (streamoo_network_state_t){0};
  return false;
}
#endif
