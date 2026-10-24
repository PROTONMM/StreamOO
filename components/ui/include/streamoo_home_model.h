#pragma once
#include "streamoo_visualization.h"

typedef struct { const uint8_t *data; size_t bytes; } streamoo_ui_text_t;
typedef struct {
    uint16_t year;
    uint8_t month, day, hour, minute, second;
} streamoo_ui_clock_t;

/* Coherent immutable producer snapshot, borrowed strings live until the next
 * home update. Increment stream_generation when station/stream changes.
 * Within one stream, increment text_revision whenever any displayed string changes.
 * All stream-derived data share data_station_id/data_generation tags.
 * Network and civil time are independent; no timezone/SNTP work occurs in UI. */
typedef struct {
    uint64_t station_id, stream_generation;
    uint64_t data_station_id, data_generation, text_revision;
    streamoo_ui_text_t station_name, title, description, codec;
    uint32_t bitrate_kbps; /* 0 = unknown, even if codec is known. */
    int16_t rssi_dbm;
    bool wifi_connected, rssi_valid, metadata_valid, audio_info_valid;
    bool clock_valid, visualization_valid;
    streamoo_ui_clock_t clock;
    streamoo_visualization_model_t visualization;
} streamoo_home_snapshot_t;
