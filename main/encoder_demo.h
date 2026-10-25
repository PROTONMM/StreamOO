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
