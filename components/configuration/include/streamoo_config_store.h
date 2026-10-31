#pragma once
#include "esp_err.h"
#include "streamoo_config_model.h"
/* Task-only persistence. Call init once before exposing write routes. */
esp_err_t streamoo_config_store_init(void);
bool streamoo_config_wifi_load(streamoo_wifi_profiles_t *);
esp_err_t streamoo_config_wifi_save(const streamoo_wifi_profiles_t *);
/* Validates/normalizes an upload REQUEST COPY; commit atomically renames in
 * LittleFS. */
esp_err_t streamoo_config_playlist(uint8_t *, size_t, bool,
                                   streamoo_playlist_report_t *);
bool streamoo_config_playlist_info(streamoo_playlist_report_t *);

#include "streamoo_stations.h"
typedef struct streamoo_catalog_snapshot streamoo_catalog_snapshot_t;
typedef struct {
  const streamoo_station_t *stations;
  uint32_t count;
  uint64_t generation;
} streamoo_catalog_view_t;
/* O(1), no flash I/O: immutable, refcounted snapshots, also during upload.
 * Each acquired reference must be released; view stays valid until release. */
streamoo_catalog_snapshot_t *streamoo_config_catalog_acquire(void);
streamoo_catalog_view_t
streamoo_config_catalog_view(const streamoo_catalog_snapshot_t *);
void streamoo_config_catalog_release(streamoo_catalog_snapshot_t *);
