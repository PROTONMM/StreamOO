#include "streamoo_settings.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static SemaphoreHandle_t writes;
static bool ready;
static streamoo_settings_t current = {.list_timeout_s = 5,
                                      .list_autoselect = true, .autoplay = true};
static uint64_t last_station, pending_station, station_revision;
static int64_t station_at;
bool streamoo_settings_valid(const streamoo_settings_t *s) {
  if (!s || s->list_timeout_s < 1 || s->list_timeout_s > 120)
    return false;
  size_t n = 0;
  while (n < sizeof s->hostname && s->hostname[n]) {
    char c = s->hostname[n];
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'))
      return false;
    ++n;
  }
  return n < sizeof s->hostname &&
         (!n || (s->hostname[0] != '-' && s->hostname[n - 1] != '-'));
}
streamoo_settings_t streamoo_settings_get(void) {
  portENTER_CRITICAL(&lock);
  streamoo_settings_t out = current;
  portEXIT_CRITICAL(&lock);
  return out;
}
uint64_t streamoo_settings_last_station(void) {
  portENTER_CRITICAL(&lock);
  uint64_t id = last_station;
  portEXIT_CRITICAL(&lock);
  return id;
}
void streamoo_settings_station_changed(uint64_t id) {
  if (!ready)
    return;
  portENTER_CRITICAL(&lock);
  if (pending_station != id) {
    pending_station = id;
    ++station_revision;
    station_at = esp_timer_get_time();
  }
  portEXIT_CRITICAL(&lock);
}
/* Explicit byte encoding: no structure padding or ABI-dependent persistence. */
static void encode(const streamoo_settings_t *s, uint8_t data[68]) {
  memset(data, 0, 68);
  data[0] = 2;
  data[1] = (uint8_t)s->list_timeout_s;
  data[2] = s->list_autoselect;
  data[3] = s->autoplay;
  memcpy(data + 4, s->hostname, 64);
}
esp_err_t streamoo_settings_set(const streamoo_settings_t *s) {
  if (!streamoo_settings_valid(s))
    return ESP_ERR_INVALID_ARG;
  if (!ready || !writes)
    return ESP_ERR_INVALID_STATE;
  if (xSemaphoreTake(writes, pdMS_TO_TICKS(5000)) != pdTRUE)
    return ESP_ERR_TIMEOUT;
  uint8_t data[68];
  encode(s, data);
  nvs_handle_t h;
  esp_err_t error = nvs_open("streamoo_system", NVS_READWRITE, &h);
  if (error == ESP_OK) {
    error = nvs_set_blob(h, "settings", data, sizeof data);
    if (error == ESP_OK)
      error = nvs_commit(h);
    nvs_close(h);
  }
  if (error == ESP_OK) {
    portENTER_CRITICAL(&lock);
    current = *s;
    portEXIT_CRITICAL(&lock);
  }
  xSemaphoreGive(writes);
  return error;
}
static void worker(void *context) {
  (void)context;
  uint64_t saved = 0;
  int64_t retry_at = 0;
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(250));
    uint64_t revision, id;
    int64_t changed;
    portENTER_CRITICAL(&lock);
    revision = station_revision;
    id = pending_station;
    changed = station_at;
    portEXIT_CRITICAL(&lock);
    int64_t now = esp_timer_get_time();
    if (revision == saved || now - changed < 5000000 || now < retry_at)
      continue;
    if (xSemaphoreTake(writes, pdMS_TO_TICKS(100)) != pdTRUE)
      continue;
    nvs_handle_t h;
    esp_err_t error = nvs_open("streamoo_system", NVS_READWRITE, &h);
    if (error == ESP_OK) {
      error = nvs_set_u64(h, "station", id);
      if (error == ESP_OK)
        error = nvs_commit(h);
      nvs_close(h);
    }
    if (error == ESP_OK) {
      saved = revision;
      portENTER_CRITICAL(&lock);
      last_station = id;
      portEXIT_CRITICAL(&lock);
    } else
      retry_at = now + 5000000;
    xSemaphoreGive(writes);
  }
}
esp_err_t streamoo_settings_start(void) {
  if (writes)
    return ESP_ERR_INVALID_STATE;
  esp_err_t error = nvs_flash_init();
  if (error != ESP_OK)
    return error;
  writes = xSemaphoreCreateMutex();
  if (!writes)
    return ESP_ERR_NO_MEM;
  nvs_handle_t h;
  error = nvs_open("streamoo_system", NVS_READONLY, &h);
  if (error == ESP_OK) {
    uint8_t data[68];
    size_t bytes = sizeof data;
    esp_err_t found = nvs_get_blob(h, "settings", data, &bytes);
    if (found == ESP_OK && bytes == sizeof data && (data[0] == 1 || data[0] == 2) &&
        data[2] <= 1 && data[3] <= 1) {
      streamoo_settings_t loaded = {.list_timeout_s = data[1],
                                    .list_autoselect = data[2] != 0,
                                    .autoplay = data[0] == 1 || data[3] != 0};
      memcpy(loaded.hostname, data + 4, 64);
      if (streamoo_settings_valid(&loaded))
        current = loaded;
    }
    uint64_t id = 0;
    if (nvs_get_u64(h, "station", &id) == ESP_OK)
      last_station = pending_station = id;
    nvs_close(h);
  } else if (error != ESP_ERR_NVS_NOT_FOUND)
    return error;
  ready = true;
  if (xTaskCreate(worker, "system_settings", 4096, NULL, 1, NULL) != pdPASS) {
    ready = false;
    return ESP_ERR_NO_MEM;
  }
  return ESP_OK;
}
