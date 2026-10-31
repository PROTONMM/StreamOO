#pragma once
#include "streamoo_ui_controls.h"

typedef struct {
    streamoo_ui_t ui;
    uint64_t station_id, generation;
    int volume;
    bool running, selection_pending, control_pending, buffering;
    streamoo_ui_select_command_t selection;
    streamoo_control_command_t control;
    streamoo_control_post_fn audio_post;
    void *audio_context;
    streamoo_ui_text_t audio_title;
    streamoo_ui_clock_t clock;
    bool clock_valid;
    char network_ip[16], codec[8];
    bool wifi_connected, rssi_valid;
    int rssi_dbm;
    uint32_t bitrate_kbps;
    bool imported_catalog, levels_valid;
    streamoo_visualization_model_t levels;
    streamoo_ui_post_fn station_post;
    void *station_context;
    char player_text[257];
} streamoo_encoder_demo_t;
bool streamoo_encoder_demo_init(streamoo_encoder_demo_t *demo);
bool streamoo_encoder_demo_input(streamoo_encoder_demo_t *demo,
                                  streamoo_input_event_t event,
                                  streamoo_control_mode_t mode, uint64_t now);
bool streamoo_encoder_demo_draw(streamoo_encoder_demo_t *demo, uint8_t *frame,
                                 size_t bytes, uint64_t now);

/* Optional external command owner; default unbound demo remains host-testable. */
bool streamoo_encoder_demo_bind_audio(streamoo_encoder_demo_t *demo,
    streamoo_control_post_fn post, void *context);
bool streamoo_encoder_demo_audio_state(streamoo_encoder_demo_t *demo, uint8_t volume, bool running);

/* Copied address for the third HOME row; NULL clears it. */
bool streamoo_encoder_demo_set_ip(streamoo_encoder_demo_t *, const char *);

/* Borrowed immutable UI projection; clears stale HOME text before old release. */
bool streamoo_encoder_demo_set_catalog(streamoo_encoder_demo_t *, const streamoo_ui_station_t *, uint32_t, uint64_t);

void streamoo_encoder_demo_bind_station_owner(streamoo_encoder_demo_t *, streamoo_ui_post_fn, void *);

/* Copies live visualization levels; NULL invalidates. No fabricated bands. */
bool streamoo_encoder_demo_levels(streamoo_encoder_demo_t *, const streamoo_visualization_model_t *);

/* Copies technical display data; NULL codec clears unknown format. */
bool streamoo_encoder_demo_technical(streamoo_encoder_demo_t *, bool connected,
    bool rssi_valid, int rssi_dbm, const char *codec);
