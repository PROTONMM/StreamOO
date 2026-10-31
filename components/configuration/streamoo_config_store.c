#include "streamoo_config_store.h"
#include "esp_heap_caps.h"
#include "esp_littlefs.h"
#include "esp_partition.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
static SemaphoreHandle_t mutex;
static bool mounted;
static streamoo_playlist_report_t catalog;
static bool take(void) {
  return mutex && xSemaphoreTake(mutex, pdMS_TO_TICKS(5000)) == pdTRUE;
}
static bool blank_partition(void) {
  const esp_partition_t *p = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "storage");
  if (!p)
    return false;
  uint8_t buffer[1024];
  for (size_t pos = 0; pos < p->size; pos += sizeof buffer) {
    if (esp_partition_read(p, pos, buffer, sizeof buffer) != ESP_OK)
      return false;
    for (unsigned i = 0; i < sizeof buffer; ++i)
      if (buffer[i] != 255)
        return false;
  }
  return true;
}
static esp_err_t mount_storage(void) {
  esp_vfs_littlefs_conf_t config = {.base_path = "/stations",
                                    .partition_label = "storage",
                                    .format_if_mount_failed = false};
  esp_err_t error = esp_vfs_littlefs_register(&config);
  if (error != ESP_OK && error != ESP_FAIL)
    return error;
  nvs_handle_t handle;
  if (nvs_open("streamoo_fs", NVS_READWRITE, &handle) != ESP_OK)
    return error;
  uint8_t phase = 0;
  esp_err_t found = nvs_get_u8(handle, "init", &phase);
  bool permission = (found == ESP_OK && phase == 1) ||
                    (found == ESP_ERR_NVS_NOT_FOUND && blank_partition());
  if (error == ESP_OK) {
    error = nvs_set_u8(handle, "init", 2);
    if (error == ESP_OK)
      error = nvs_commit(handle);
  } else if (permission) {
    error = nvs_set_u8(handle, "init", 1);
    if (error == ESP_OK)
      error = nvs_commit(handle);
    if (error == ESP_OK)
      error = esp_littlefs_format("storage");
    if (error == ESP_OK)
      error = esp_vfs_littlefs_register(&config);
    if (error == ESP_OK) {
      error = nvs_set_u8(handle, "init", 2);
      if (error == ESP_OK)
        error = nvs_commit(handle);
    }
  }
  nvs_close(handle);
  return error;
}
esp_err_t streamoo_config_store_init(void) {
  if (mutex)
    return ESP_ERR_INVALID_STATE;
  esp_err_t error = nvs_flash_init();
  if (error != ESP_OK)
    return error;
  mutex = xSemaphoreCreateMutex();
  if (!mutex)
    return ESP_ERR_NO_MEM;
  error = mount_storage();
  mounted = error == ESP_OK;
  if (mounted) {
    FILE *file = fopen("/stations/playlist.csv", "rb");
    if (file) {
      fseek(file, 0, SEEK_END);
      long size = ftell(file);
      rewind(file);
      if (size > 0 && size <= STREAMOO_PLAYLIST_MAX_BYTES) {
        uint8_t *data =
            heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (data && fread(data, 1, size, file) == (size_t)size) {
          streamoo_playlist_report_t report = {0};
          if (streamoo_config_playlist(data, size, false, &report) == ESP_OK)
            catalog = report;
          else
            catalog.error = "stored_catalog_invalid";
        }
        heap_caps_free(data);
      }
      fclose(file);
    }
  }
  return error;
}
bool streamoo_config_wifi_load(streamoo_wifi_profiles_t *out) {
  if (!out || !take())
    return false;
  *out = (streamoo_wifi_profiles_t){0};
  nvs_handle_t handle;
  esp_err_t error = nvs_open("streamoo_net", NVS_READONLY, &handle);
  if (error == ESP_ERR_NVS_NOT_FOUND) {
    xSemaphoreGive(mutex);
    return true;
  }
  bool ok = false;
  if (error == ESP_OK) {
    uint8_t bytes[1024];
    size_t n = sizeof bytes;
    error = nvs_get_blob(handle, "profiles", bytes, &n);
    ok = error == ESP_ERR_NVS_NOT_FOUND ||
         (error == ESP_OK && streamoo_wifi_profiles_decode(bytes, n, out));
    nvs_close(handle);
  }
  xSemaphoreGive(mutex);
  return ok;
}
esp_err_t streamoo_config_wifi_save(const streamoo_wifi_profiles_t *profiles) {
  uint8_t data[1024];
  size_t bytes = streamoo_wifi_profiles_encode(profiles, data, sizeof data);
  if (!bytes)
    return ESP_ERR_INVALID_ARG;
  if (!take())
    return ESP_ERR_TIMEOUT;
  nvs_handle_t handle;
  esp_err_t error = nvs_open("streamoo_net", NVS_READWRITE, &handle);
  if (error == ESP_OK) {
    error = nvs_set_blob(handle, "profiles", data, bytes);
    if (error == ESP_OK)
      error = nvs_commit(handle);
    nvs_close(handle);
  }
  memset(data, 0, sizeof data);
  xSemaphoreGive(mutex);
  return error;
}
esp_err_t streamoo_config_playlist(uint8_t *data, size_t bytes, bool commit,
                                   streamoo_playlist_report_t *report) {
  if (!report)
    return ESP_ERR_INVALID_ARG;
  streamoo_playlist_workspace_t *ws =
      heap_caps_calloc(1, sizeof(*ws), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!ws)
    return ESP_ERR_NO_MEM;
  bool valid = streamoo_playlist_validate(data, &bytes, report, ws);
  heap_caps_free(ws);
  if (!valid)
    return ESP_ERR_INVALID_ARG;
  if (!commit)
    return ESP_OK;
  if (!take())
    return ESP_ERR_TIMEOUT;
  esp_err_t error = ESP_FAIL;
  if (mounted) {
    FILE *file = fopen("/stations/playlist.tmp", "wb");
    if (file) {
      bool written = fwrite(data, 1, bytes, file) == bytes;
      if (fflush(file) != 0 || fsync(fileno(file)) != 0)
        written = false;
      if (fclose(file) != 0)
        written = false;
      if (written &&
          rename("/stations/playlist.tmp", "/stations/playlist.csv") == 0) {
        catalog = *report;
        error = ESP_OK;
      } else
        unlink("/stations/playlist.tmp");
    }
  }
  xSemaphoreGive(mutex);
  return error;
}
bool streamoo_config_playlist_info(streamoo_playlist_report_t *out) {
  if (!out || !take())
    return false;
  *out = catalog;
  bool ok = mounted;
  xSemaphoreGive(mutex);
  return ok;
}
